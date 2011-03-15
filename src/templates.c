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

#include "libs/pcre.h"
#include "common_functions.h"

typedef struct
{
	u_str_t name;
	u_str_t value;
} tpl_var_t;

typedef struct
{
	u_str_t name;
	u_str_t data;
	void * next;
} tpl_cont_t;

static buf_t * static_vars;

static char * re_pattern_var = "\\$\\{(\\w+)\\}";
static char * re_pattern_action = "\\{(\\w+) *= *('|\"|)([^\\2]+?)\\2\\}";

static pcre * re_var;
static pcre * re_action;


static inline void _tpl_set_var (buf_t * vars, const char * name, uchar * value, uint len)
{
	uint i;
	uint name_len = strlen(name);
	tpl_var_t * var;
	
	for (i = 0; i < vars->cur_len; i++)
	{
		var = &(((tpl_var_t *) vars->data)[i]);
		if (name_len == var->name.len && memcmp(name, var->name.str, name_len) == 0)
		{
			var->value.str = (uchar *) realloc(var->value.str, len);
			memcpy(var->value.str, value, len);
			var->value.len = len;
			
			return;
		}
	}
	
	buf_expand(vars, 1);
	var = &(((tpl_var_t *) vars->data)[vars->cur_len - 1]);
	var->name.str = (uchar *) malloc(name_len + 1);
	memcpy(var->name.str, name, name_len + 1);
	var->name.len = name_len;
	var->value.str = (uchar *) malloc(len);
	memcpy(var->value.str, value, len);
	var->value.len = len;
}

void tpl_set_static_var (const char * name, uchar * value, uint len)
{
	_tpl_set_var(static_vars, name, value, len);
}

static void tpl_process_file (tpl_cont_t ** tpl, const char * file)
{
	FILE * f = fopen(file, "r");
	if (f == NULL)
		return;
	uchar buf[READ_SIZE];
	int count;
	uchar * tmpbuf = NULL;
	uint tmpbuf_len = 0;
	while ((count = fread(buf, 1, READ_SIZE, f)) > 0)
	{
		tmpbuf_len += count;
		tmpbuf = realloc(tmpbuf, tmpbuf_len);
		memcpy(tmpbuf, buf + (tmpbuf_len - count), count);
	}
	fclose(f);
	if (tmpbuf == NULL)
		return;
	tpl_cont_t * t = (tpl_cont_t *) malloc(sizeof(tpl_cont_t));
	t->next = NULL;
	
	int rc, offset = 0;
	int ovector[15];
	uchar * act, * prm;
	
	for (;;)
	{
		rc = pcre_exec(re_action, NULL, (const char *) tmpbuf, tmpbuf_len, offset, 0, ovector, 15);
		if (rc < 4)
			break;
		offset = ovector[1];
		act = tmpbuf + ovector[2];
		act[ovector[3] - ovector[2]] = '\0';
		prm = tmpbuf + ovector[6];
		prm[ovector[7] - ovector[6]] = '\0';
		debug_print_3("%s %s", act, prm);
		if (strcmp((char *) act, "include") == 0)
		{
			
		}
	}
	
	free(tmpbuf);
	* tpl = t;
}

tpl_cont_t * tpl_load (const char * file)
{
	tpl_cont_t * tpl = NULL;
	
	tpl_process_file(&tpl, file);
	
	return tpl;
}

void tpl_init (void)
{
	static_vars = buf_create(sizeof(tpl_var_t), 1);
	const char * errptr;
	int erroffset;
	re_var = pcre_compile(re_pattern_var, PCRE_MULTILINE, &errptr, &erroffset, NULL);
	re_action = pcre_compile(re_pattern_action, PCRE_MULTILINE, &errptr, &erroffset, NULL);
	if (re_var == NULL || re_action == NULL)
		eerr(0, "pcre_compile(): error \"%s\" in offset %d", errptr, erroffset);
	
	//////
	// XXX
	//////
	tpl_load("web/templates/default/main.tpl");
}
