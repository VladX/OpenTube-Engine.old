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

#include <libintl.h>
#include <locale.h>
#include "common_functions.h"
#include "lang/strings.h"

static const uchar ** lstrings;

const uchar * localization_get_string (uint id)
{
	return lstrings[id];
}

static void localization_load_strings (void)
{
	uint i, len = ARRAY_LENGTH(strings);
	
	for (i = 0; i < len; i++)
	{
		lstrings[i] = (const uchar *) strings[i];
		if (strings[i] == NULL)
			continue;
		lstrings[i] = (const uchar *) gettext(strings[i]);
	}
}

void localization_init (void)
{
	#ifndef _WIN
	setlocale(LC_ALL, "");
	#endif
	bindtextdomain(GETTEXT_DOMAIN, LOCALE_DIR);
	textdomain(GETTEXT_DOMAIN);
	lstrings = malloc(sizeof(strings));
	localization_load_strings();
}
