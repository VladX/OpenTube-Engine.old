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
#include "web/callbacks.h"

buf_t * uri_map;
buf_t * web_global_buffer;

void web_set_callback (const char * uri, web_func_t func)
{
	buf_expand(uri_map, 1);
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].uri = uri;
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].func = func;
}

void web_init (void)
{
	uri_map = buf_create(sizeof(uri_map_t), 10);
	web_global_buffer = buf_create(1, WEB_GLOBAL_BUFFER_RESERVED_SIZE);
	
	set_callbacks();
}

void web_cleanup (void)
{
	buf_free(web_global_buffer);
}
