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

#include <pthread.h>
#include <sys/stat.h>
#include <map>
#include <string>
#include <CTPP2OutputCollector.hpp>
#include <CTPP2StringOutputCollector.hpp>
#include <CTPP2SyscallFactory.hpp>
#include <CTPP2VMSTDLib.hpp>
#include <CTPP2Parser.hpp>
#include <CTPP2FileSourceLoader.hpp>
#include <CTPP2ParserException.hpp>
#include <CTPP2HashTable.hpp>
#include <CTPP2VM.hpp>
#include <CTPP2VMDumper.hpp>
#include <CTPP2VMOpcodes.h>
#include <CTPP2VMMemoryCore.hpp>
#include <CTPP2FileLogger.hpp>

using namespace CTPP;

extern "C"
{

#include "common_functions.h"

#ifdef _WIN
#include <io.h>
#endif

typedef struct
{
	const VMExecutable * exec;
	const VMMemoryCore * vmmemcore;
	std::string data;
	u_str_t * out;
	time_t mtime;
	UINT_32 size;
} compiled_template;

struct thread_context
{
	pthread_t id;
	SyscallFactory * oSyscallFactory;
	VM * oVM;
	FileLogger * oLogger;
};

typedef std::map <std::string, compiled_template *> ct_map;

static const char * tplwdir = "/home/vlad/src/OpenTube-Engine/web/templates/default/";

static struct thread_context ctx[WORKER_THREADS];
static ct_map compiled_templates;
static CDT oHash;
static pthread_mutex_t mutex_access[1];

#undef error_in_file
#define error_in_file(PATTERN, ...) eerr(0, "An error occurred while trying to parse \"%s\": " PATTERN, file, __VA_ARGS__)

#ifdef _WIN
 static pthread_t _ptnew;
 #define pthread_t_cmp(X, Y) pthread_equal(X, Y)
 #define pthread_t_isnew(X) memcmp(&_ptnew, &(X), sizeof(_ptnew))
 #define pthread_t_setnew(X) memset(&(X), 0, sizeof(_ptnew))
#else
 #define pthread_t_cmp(X, Y) X == Y
 #ifdef _BSD
  #define pthread_t_isnew(X) X == NULL
  #define pthread_t_setnew(X) X = NULL
 #else
  #define pthread_t_isnew(X) X == (unsigned int) -1
  #define pthread_t_setnew(X) X = -1
 #endif
#endif


void ctpp_set_var (const char * name, const char * value)
{
	oHash[name] = value;
}

void ctpp_run (const char * file, u_str_t ** out)
{
	pthread_mutex_lock(mutex_access);
	static pthread_t id;
	id = pthread_self();
	static uint i;
	for (i = 0; i < WORKER_THREADS; i++)
		if (pthread_t_cmp(ctx[i].id, id))
			break;
		else if (pthread_t_isnew(ctx[i].id))
		{
			ctx[i].id = id;
			break;
		}
	
	struct thread_context * c = &(ctx[i]);
	
	static ct_map::iterator it;
	static std::string mkey;
	
	mkey = file;
	it = compiled_templates.find(mkey);
	
	if (it == compiled_templates.end())
	{
		pthread_mutex_unlock(mutex_access);
		* out = NULL;
		return;
	}
	
	compiled_template * ct;
	ct = (* it).second;
	
	pthread_mutex_unlock(mutex_access);
	
	ct->data.clear();
	StringOutputCollector oOutputCollector(ct->data);
	
	c->oVM->Init(ct->vmmemcore, &oOutputCollector, c->oLogger);
	UINT_32 iIP = 0;
	c->oVM->Run(ct->vmmemcore, &oOutputCollector, iIP, oHash, c->oLogger);
	
	ct->out->len = ct->data.size();
	ct->out->str = (uchar *) ct->data.data();
	
	* out = ct->out;
}

void ctpp_compile (const char * file)
{
	pthread_mutex_lock(mutex_access);
	
	static struct stat st;
	
	int r;
	char * cwd = gnu_getcwd();
	
	if (cwd != NULL)
	{
		r = chdir(tplwdir);
		if (r == -1)
			peerr(0, "chdir(%s)", tplwdir);
	}
	
	if (stat(file, &st) == -1)
		peerr(0, "stat(%s)", file);
	
	if (cwd != NULL)
	{
		r = chdir(cwd);
		if (r == -1)
			peerr(0, "chdir(%s)", cwd);
		free(cwd);
	}
	
	std::string mkey = file;
	
	static ct_map::iterator it;
	it = compiled_templates.find(mkey);
	if (it != compiled_templates.end())
	{
		if (st.st_mtime == ((* it).second)->mtime)
		{
			pthread_mutex_unlock(mutex_access);
			return;
		}
		((* it).second)->data.clear();
		delete ((* it).second)->vmmemcore;
		free((void *) ((* it).second)->exec);
		delete ((* it).second)->out;
		delete (* it).second;
	}
	
	VMOpcodeCollector oVMOpcodeCollector;
	StaticText oSyscalls;
	StaticData oStaticData;
	StaticText oStaticText;
	HashTable oHashTable;
	
	CTPP2Compiler oCompiler(oVMOpcodeCollector, oSyscalls, oStaticData, oStaticText, oHashTable);
	
	try
	{
		CTPP2FileSourceLoader oSourceLoader;
		
		std::string includedir = tplwdir;
		const std::vector <std::string> includedirs(&includedir, &includedir + 1);
		oSourceLoader.SetIncludeDirs(includedirs);
		oSourceLoader.LoadTemplate(file);
		
		CTPP2Parser oCTPP2Parser(&oSourceLoader, &oCompiler, file);
		oCTPP2Parser.Compile();
	}
	catch (CTPPParserSyntaxError & e)
	{
		error_in_file("At line %d, pos. %d: %s", e.GetLine(), e.GetLinePos(), e.what());
	}
	catch (CTPPParserOperatorsMismatch & e)
	{
		error_in_file("At line %d, pos. %d: expected %s, but found </%s>", e.GetLine(), e.GetLinePos(), e.Expected(), e.Found());
	}
	catch (CTPPLogicError & e)
	{
		error_in_file("%s", e.what());
	}
	catch (CTPPUnixException & e)
	{
		error_in_file("I/O in %s: %s", e.what(), strerror(e.ErrNo()));
	}
	catch (...)
	{
		error_in_file("%s", "Unknown error");
	}
	
	UINT_32 iCodeSize = 0;
	const VMInstruction * oVMInstruction = oVMOpcodeCollector.GetCode(iCodeSize);
	VMDumper oDumper(iCodeSize, oVMInstruction, oSyscalls, oStaticData, oStaticText, oHashTable);
	UINT_32 iSize = 0;
	const VMExecutable * aProgramCore = oDumper.GetExecutable(iSize);
	compiled_template * ct = new compiled_template;
	ct->exec = (const VMExecutable *) malloc(iSize);
	memcpy((void*) ct->exec, aProgramCore, iSize);
	ct->size = iSize;
	ct->vmmemcore = new VMMemoryCore(ct->exec);
	ct->out = new u_str_t;
	ct->mtime = st.st_mtime;
	compiled_templates[mkey] = ct;
	pthread_mutex_unlock(mutex_access);
}

void ctpp_init (void)
{
	#ifdef _WIN
	memset(&_ptnew, 0, sizeof(_ptnew));
	#endif
	pthread_mutex_init(mutex_access, NULL);
	uint i;
	for (i = 0; i < WORKER_THREADS; i++)
	{
		ctx[i].oSyscallFactory = new SyscallFactory(WORKER_THREADS);
		STDLibInitializer::InitLibrary(* (ctx[i].oSyscallFactory));
		ctx[i].oVM = new VM(ctx[i].oSyscallFactory);
		ctx[i].oLogger = new FileLogger(stderr);
		pthread_t_setnew(ctx[i].id);
	}
}

}
