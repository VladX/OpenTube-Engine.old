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

#define HTTP_HTML_PAGE_TEMPLATE_TOP \
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" \
	"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n" \
	"<head>\n" \
	"	<title>"

#define HTTP_HTML_PAGE_TEMPLATE_CON1 \
	"</title>\n" \
	"	<style type=\"text/css\">\n" \
	"		html, body, h1, h2 {padding:0;margin:0;} body {background:#eee;} h1 {color:#943535;font-family:\"Times New Roman\";font-size:128px;padding-top:40px;} h2, span {font-family:Arial;text-shadow:1px 1px 0 #fff;} h2 {color:#444;font-size:35px;} .server-info {color:#888;font-size:14px;}\n" \
	"	</style>\n" \
	"</head>\n\n" \
	"<body>\n" \
	"	<div align=\"center\">\n" \
	"		<h1>"

#define HTTP_HTML_PAGE_TEMPLATE_CON2 \
	"</h1>\n" \
	"		<h2>"

#define HTTP_HTML_PAGE_TEMPLATE_BOT \
	"</h2>\n" \
	"		<br />\n" \
	"		<span class=\"server-info\">" SERVER_STRING "</span>\n" \
	"	</div>\n" \
	"</body>\n" \
	"</html>\n"

#define HTTP_ERROR_CONTENT_TYPE "text/html"
#define HTTP_ERROR_CONTENT_TYPE_LEN 9
