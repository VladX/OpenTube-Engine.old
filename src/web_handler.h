/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef E_WEB_HANDLER_H
#define E_WEB_HANDLER_H 1

#include "common_functions.h"
#include "templates.h"
#include "localization.h"

#define WEB_CALLBACK_TO_C_FUNC(NAME) web_cb__##NAME
#define WEB_INIT_CALLBACK_TO_C_FUNC(NAME) web_init_cb__##NAME

#define WEB_CALLBACK(NAME, URI, FULL_MATCH) buf_t * WEB_CALLBACK_TO_C_FUNC(NAME) (void)
#define WEB_INIT(NAME) void WEB_INIT_CALLBACK_TO_C_FUNC(NAME) (void)

extern threadsafe buf_t * thread_global_buffer;
extern threadsafe request_t * thread_request;

void http_parse_query_string (request_t * r);

void http_parse_cookies (request_t * r);

void http_parse_post (request_t * r);

void web_raise (ushort code);

#endif
