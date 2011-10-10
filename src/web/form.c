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

WEB_CALLBACK(form, "/form", true)
{
	PRINT("<form enctype=\"multipart/form-data\" method=\"post\" action=\"\">\
		<input type=\"file\" name=\"code_img\">\
		<input type=\"text\" name=\"captcha\">\
	</form>");
	
	//http_parse_query_string(r);
	//http_parse_cookies(r);
	http_parse_post(thread_request);
	
	return thread_global_buffer;
}
