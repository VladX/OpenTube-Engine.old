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

#include <web_handler.h>
#include "../util.c"


/* Automatically inserted by script generate-callbacks.c.py */
#include "../form.c"
#include "../hello.c"


#include <web.h>

void set_callbacks (void)
{
	/* Automatically inserted by script generate-callbacks.c.py */
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(form), "/form", true);
	web_set_callback(WEB_CALLBACK_TO_C_FUNC(hello), "/hello", true);
}

void run_init_callbacks (void)
{
	/* Automatically inserted by script generate-callbacks.c.py */
}
