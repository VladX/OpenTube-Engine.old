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
#include "templates_ctpp.h"

void tpl_set_var (const char * name, const char * value)
{
	ctpp_set_var(name, value);
}

u_str_t * tpl_load (const char * file)
{
	u_str_t * out;
	
	if (config.tpl_cache_update)
	{
		ctpp_compile(file);
		ctpp_run(file, &out);
	}
	else
	{
		ctpp_run(file, &out);
		if (out == NULL)
		{
			ctpp_compile(file);
			ctpp_run(file, &out);
		}
	}
	
	return out;
}

void tpl_init (void)
{
	ctpp_init();
}
