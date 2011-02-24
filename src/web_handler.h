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

#ifndef E_WEB_HANDLER_H

#include "common_functions.h"

#define E_WEB_HANDLER_H

#define WEB_FUNCTION(NAME, URI, STRICT_COMPARSION) buf_t * NAME (request_t * r)

extern buf_t * web_global_buffer;

void http_parse_query_string (request_t * r);

void http_parse_cookies (request_t * r);

void http_parse_post (request_t * r);

#endif
