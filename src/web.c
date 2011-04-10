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

#include <setjmp.h>
#include "common_functions.h"
#include "templates.h"
#include "web/callbacks/callbacks.h"

buf_t * uri_map;
threadsafe buf_t * thread_global_buffer;
threadsafe request_t * thread_request;
threadsafe jmp_buf web_exceptions_jmpbuf;
threadsafe volatile bool thread_allow_compression;

void web_set_callback (web_func_t func, const char * uri, bool full_match)
{
	buf_expand(uri_map, 1);
	uri_map_t * m = &(((uri_map_t *) uri_map->data)[uri_map->cur_len - 1]);
	m->uri.str = (uchar *) uri;
	m->uri.len = strlen(uri);
	m->func = func;
	m->strict_comparison = (m->uri.str[m->uri.len - 1] == '/') ? false : true;
	m->full_match = full_match;
}

void web_setup_global_buffer (request_t * r)
{
	thread_allow_compression = true;
	thread_request = r;
	thread_global_buffer = r->out_data;
	buf_free(thread_global_buffer);
}

void web_raise (ushort code)
{
	longjmp(web_exceptions_jmpbuf, code);
}

void web_init (void)
{
	uri_map = buf_create(sizeof(uri_map_t), 10);
	
	tpl_init();
	set_callbacks();
}
