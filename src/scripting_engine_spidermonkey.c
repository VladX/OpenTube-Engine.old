/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - VladX; http://vladx.net/
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "config.h"

#ifdef HAVE_SPIDERMONKEY_ENGINE

#define DO_NOT_DEFINE_64BIT_TYPES 1
#include <jsapi.h>
#include "common_functions.h"
#include "os_stat.h"
#include "web.h"
#include "web_handler.h"

#ifndef JS_THREADSAFE
 #error "SpiderMonkey must be built with threadsafe option enabled in order to work correctly."
#endif

struct js_compiled_script
{
	JSObject * script;
	char * filename;
	str_t uri;
	time_t mtime;
	bool full_match;
};

static JSRuntime * jsrt = NULL;
static threadsafe JSContext * jsctx = NULL;
static threadsafe JSObject * jsglobal;
static threadsafe buf_t * compiled_scripts = NULL;
static const char * scripts_dir = "/scripts/";
static const uint jsmaxbytes = 4 * 1024 * 1024;

static JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub, JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool js_function_map_uri (JSContext *, uintN, jsval *);
static JSBool js_function_print (JSContext *, uintN, jsval *);
static JSObject * js_compile (const char *, JSObject *, time_t *);

static JSFunctionSpec js_global_functions[] = {
	JS_FS("map_uri", js_function_map_uri, 3, 0),
	JS_FS("print", js_function_print, 1, 0),
	JS_FS("echo", js_function_print, 1, 0),
	JS_FS_END
};

static void js_update_bytecode (struct js_compiled_script * cs)
{
	if (config.script_update == 0)
		return;
	
	cs->script = js_compile(cs->filename, cs->script, &(cs->mtime));
}

static buf_t * js_web_callback (void)
{
	uint i;
	struct js_compiled_script * cs;
	jsval retval;
	
	for (i = 0; i < compiled_scripts->cur_len; i++)
	{
		cs = &(((struct js_compiled_script *) compiled_scripts->data)[i]);
		if (cs->full_match)
		{
			if (cs->uri.len == thread_request->in.path.len && memcmp(cs->uri.str, thread_request->in.path.str, cs->uri.len) == 0)
			{
				JS_BeginRequest(jsctx);
				js_update_bytecode(cs);
				JS_ExecuteScript(jsctx, jsglobal, cs->script, &retval);
				JS_MaybeGC(jsctx);
				JS_EndRequest(jsctx);
				
				return thread_global_buffer;
			}
		}
		else
		{
			if (cs->uri.len <= thread_request->in.path.len && memcmp(cs->uri.str, thread_request->in.path.str, cs->uri.len) == 0)
			{
				JS_BeginRequest(jsctx);
				js_update_bytecode(cs);
				JS_ExecuteScript(jsctx, jsglobal, cs->script, &retval);
				JS_MaybeGC(jsctx);
				JS_EndRequest(jsctx);
				
				return thread_global_buffer;
			}
		}
	}
	
	err("Can not find script associated with \"%s\"", thread_request->in.path.str);
	web_raise(500);
	
	return thread_global_buffer;
}

static bool js_string_to_cstring (JSContext * ctx, JSString * str, str_t * dst)
{
	size_t dstlen = 0, srclen = 0;
	const jschar * src;
	
	src = JS_GetStringCharsAndLength(ctx, str, &srclen);
	
	if (!JS_EncodeCharacters(ctx, src, srclen, NULL, &dstlen))
		return false;
	
	dst->str = (char *) JS_malloc(ctx, dstlen + 1);
	
	if (dst->str == NULL)
		return false;
	
	if (!JS_EncodeCharacters(ctx, src, srclen, dst->str, &dstlen))
	{
		JS_free(ctx, dst->str);
		
		return false;
	}
	
	dst->str[dstlen] = '\0';
	dst->len = dstlen;
	
	return true;
}

static JSBool js_function_print (JSContext * ctx, uintN n, jsval * args)
{
	uint i;
	size_t dstlen, srclen;
	const jschar * src;
	JSString * unicode_str;
	
	if (!JS_ConvertArguments(ctx, n, JS_ARGV(ctx, args), "*"))
		return JS_FALSE;
	
	if (thread_global_buffer == NULL)
	{
		JS_ReportError(ctx, "This function can not be called from initial script.");
		
		return JS_FALSE;
	}
	
	for (i = 0; i < n; i++)
	{
		unicode_str = JS_ValueToString(ctx, (JS_ARGV(ctx, args))[i]);
		
		if (unicode_str == NULL)
			return JS_FALSE;
		
		src = JS_GetStringCharsAndLength(ctx, unicode_str, &srclen);
		
		if (!JS_EncodeCharacters(ctx, src, srclen, NULL, &dstlen))
			return JS_FALSE;
		
		buf_expand(thread_global_buffer, dstlen);
		
		if (!JS_EncodeCharacters(ctx, src, srclen, (char *) thread_global_buffer->data + thread_global_buffer->cur_len - dstlen, &dstlen))
			return JS_FALSE;
	}
	
	JS_SET_RVAL(ctx, args, JSVAL_VOID);
	
	return JS_TRUE;
}

static JSBool js_function_map_uri (JSContext * ctx, uintN n, jsval * args)
{
	JSString * unicode_str;
	str_t filename;
	uint i;
	time_t mtime = 0;
	JSBool matching = JS_TRUE;
	JSObject * script;
	struct js_compiled_script * cs;
	
	if (!JS_ConvertArguments(ctx, n, JS_ARGV(ctx, args), "Sb*", &unicode_str, &matching))
		return JS_FALSE;
	
	if (!js_string_to_cstring(ctx, unicode_str, &filename))
		return JS_FALSE;
	
	script = js_compile(filename.str, NULL, &mtime);
	
	if (config.script_update == 0)
	{
		JS_free(ctx, filename.str);
		filename.str = NULL;
	}
	
	if (script == NULL)
		return JS_FALSE;
	
	for (i = 2; i < n; i++)
	{
		if (!JS_ConvertArguments(ctx, n - i, JS_ARGV(ctx, &(args[i])), "S", &unicode_str))
			return JS_FALSE;
		
		buf_expand(compiled_scripts, 1);
		cs = &(((struct js_compiled_script *) compiled_scripts->data)[compiled_scripts->cur_len - 1]);
		cs->script = script;
		
		if (!js_string_to_cstring(ctx, unicode_str, &(cs->uri)))
			return JS_FALSE;
		
		if (cs->uri.str[0] != '/')
		{
			JS_ReportError(ctx, "Invalid URI \"%s\"", cs->uri);
			
			return JS_FALSE;
		}
		cs->full_match = (matching == JS_TRUE) ? true : false;
		cs->filename = filename.str;
		cs->mtime = mtime;
		web_set_callback(js_web_callback, cs->uri.str, cs->full_match);
	}
	
	JS_AddObjectRoot(ctx, &(cs->script));
	JS_SET_RVAL(ctx, args, JSVAL_VOID);
	
	return JS_TRUE;
}

static void js_report_error (JSContext * ctx, const char * message, JSErrorReport * report)
{
	err("%s:%u: %s: %s", report->filename, (uint) report->lineno, (report->flags == JSREPORT_WARNING) ? "warning" : "error", message);
}

static JSObject * js_compile (const char * filename, JSObject * orig_script, time_t * mtime)
{
	struct stat st;
	JSObject * script;
	char * full_path = (char *) alloca(config.data.len + strlen(filename) + 2);
	strcpy(full_path, (const char *) config.data.str);
	strcat(full_path, scripts_dir);
	strcat(full_path, filename);
	
	if (mtime && stat(full_path, &st) != -1)
	{
		if (likely(* mtime == st.st_mtime))
		{
			* mtime = st.st_mtime;
			
			return orig_script;
		}
		else
			* mtime = st.st_mtime;
	}
	
	script = (JSObject *) JS_CompileFile(jsctx, jsglobal, full_path);
	
	return script;
}

#endif

void scripts_sm_init (void)
{
	#ifdef HAVE_SPIDERMONKEY_ENGINE
	JSObject * initscript;
	jsval retval;
	uint32 options;
	
	compiled_scripts = buf_create(sizeof(struct js_compiled_script), 1);
	
	if (jsrt == NULL)
	{
		JS_SetCStringsAreUTF8();
		jsrt = JS_NewRuntime(jsmaxbytes);
	}
	
	if (jsrt == NULL)
		eerr(-1, "JS_NewRuntime(%d) failed.", jsmaxbytes);
	
	jsctx = JS_NewContext(jsrt, 8192);
	JS_BeginRequest(jsctx);
	options = JSOPTION_VAROBJFIX;
	#ifdef JSOPTION_JIT
	options |= JSOPTION_JIT;
	#endif
	JS_SetOptions(jsctx, options);
	JS_SetVersion(jsctx, JSVERSION_LATEST);
	JS_SetErrorReporter(jsctx, js_report_error);
	
	jsglobal = JS_NewCompartmentAndGlobalObject(jsctx, &global_class, NULL);
	
	if (jsglobal == NULL)
		eerr(-1, "%s", "JS_NewCompartmentAndGlobalObject() failed.");
	
	if (!JS_InitStandardClasses(jsctx, jsglobal))
		eerr(-1, "%s", "JS_InitStandardClasses() failed.");
	
	if (!JS_DefineFunctions(jsctx, jsglobal, js_global_functions))
		eerr(-1, "%s", "JS_DefineFunctions() failed.");
	
	initscript = js_compile(config.script_init, NULL, NULL);
	
	if (initscript == NULL)
		peerr(-1, "Can not run initial script \"%s\"", config.script_init);
	
	if (JS_ExecuteScript(jsctx, jsglobal, initscript, &retval) == JS_FALSE)
		eerr(-1, "%s", "At least one error occurred while running initial script.");
	
	JS_MaybeGC(jsctx);
	JS_EndRequest(jsctx);
	#endif
}
