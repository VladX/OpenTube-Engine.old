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

#ifndef HTTP_MIME_H
#define HTTP_MIME_H 1

struct mime_type
{
	const char * file_ext;
	const char * str;
	size_t len;
};

const struct mime_type http_mime_types[] = {
	
	/* File extension | Mime type | Mime type length */
	
	{".html", "text/html", 9},
	{".htm", "text/html", 9},
	{".shtml", "text/html", 9},
	{".css", "text/css", 8},
	{".js", "application/x-javascript", 24},
	{".png", "image/png", 9},
	{".gif", "image/gif", 9},
	{".jpg", "image/jpeg", 10},
	{".jpeg", "image/jpeg", 10},
	{".ico", "image/x-icon", 12},
	{".txt", "text/plain", 10},
	{".svg", "image/svg+xml", 13},
	{".xml", "text/xml", 8},
	{".swf", "application/x-shockwave-flash", 29},
	{".flv", "video/x-flv", 11},
	{".mp4", "video/mp4", 9},
	{".webm", "video/webm", 10},
	{".ogv", "video/ogg", 9},
	{".mp3", "audio/mpeg", 10},
	
	{"", "application/octet-stream", 24}
};

#endif
