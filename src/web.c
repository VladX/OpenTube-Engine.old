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
#include "web/callbacks/callbacks.h"

buf_t * uri_map;
buf_t * web_global_buffer;

void web_set_callback (web_func_t func, const char * uri, bool strict_comparison)
{
	buf_expand(uri_map, 1);
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].uri.str = (uchar *) uri;
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].uri.len = strlen(uri);
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].func = func;
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].strict_comparison = strict_comparison;
}

void web_setup_global_buffer (buf_t * buffer)
{
	web_global_buffer = buffer;
	buf_free(buffer);
}

void web_init (void)
{
	uri_map = buf_create(sizeof(uri_map_t), 10);
	web_global_buffer = NULL;
	
	set_callbacks();
	run_init_callbacks();
}
