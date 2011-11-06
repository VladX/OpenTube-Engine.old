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

#include <web_handler.h>
#include "util.c"


/* Automatically inserted by script generate-callbacks.c.py */
#include "../sleep.c"
#include "../echo.c"
#include "../form.c"
#include "../init.c"
#include "../hello.c"
#include "../video.c"
#include "../captcha_test.c"


#include <web.h>

void set_callbacks (void)
{
	/* Automatically inserted by script generate-callbacks.c.py */
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(sleep), "/sleep", true);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(echo), "/echo", true);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(form), "/form", true);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(hello), "/hello", true);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(video), "/v/", false);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(captcha_test), "/captcha_test", true);
}

void run_init_callbacks (void)
{
	/* Automatically inserted by script generate-callbacks.c.py */
	WEB_INIT_CALLBACK_TO_C_FUNC(init) ();
	WEB_INIT_CALLBACK_TO_C_FUNC(init_once) ();
	WEB_INIT_CALLBACK_TO_C_FUNC(video) ();
	WEB_INIT_CALLBACK_TO_C_FUNC(captcha_test) ();
}
