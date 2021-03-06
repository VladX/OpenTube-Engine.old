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

#include "utils/captcha.h"

WEB_CALLBACK(captcha_test, "/captcha_test", true)
{
	struct captcha_output captcha;
	
	if (captcha_generate(&captcha) == NULL)
		internal_server_error();
	
	APPEND(captcha.png_data.str, captcha.png_data.len);
	debug_print_3("captcha: %s", captcha.keyword.str);
	
	set_content_type((uchar *) "image/png", 9);
	disable_page_compression();
	
	return thread_global_buffer;
}

WEB_INIT(captcha_test)
{
	captcha_init();
}
