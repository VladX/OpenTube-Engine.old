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

#include "common_functions.h"
#include "core_http.h"
#include "core_web.h"


buf_t * test_func (request_t * r)
{
	const char * hello_world = "Hello World!";
	
	buf_t * b = buf_create(1, 0);
	int l = strlen(hello_world);
	
	b->data = (void *) hello_world;
	b->cur_len = l;
	
	return b;
}

void web_init (void)
{
	map_uri_to_function("/hello", test_func);
}
