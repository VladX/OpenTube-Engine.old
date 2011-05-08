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

char * cur_template_dir = NULL;

void tpl_set_var (const char * name, const char * value)
{
	ctpp_set_var(name, value);
}

u_str_t * tpl_load (const char * file)
{
	u_str_t * out;
	
	if (config.tpl_cache_update)
	{
		if (!ctpp_compile(file))
			return NULL;
		ctpp_run(file, &out);
	}
	else
	{
		ctpp_run(file, &out);
		if (out == NULL)
		{
			if (!ctpp_compile(file))
				return NULL;
			ctpp_run(file, &out);
		}
	}
	
	return out;
}

void tpl_init (void)
{
	if (cur_template_dir == NULL)
	{
		cur_template_dir = (char *) malloc(config.data.len + config.template_name.len + 12);
		strcpy(cur_template_dir, (char *) config.data.str);
		strcat(cur_template_dir, "/templates/");
		strcat(cur_template_dir, (char *) config.template_name.str);
		if (!is_directory_exists(cur_template_dir))
			eerr(0, "Can't access to template directory \"%s\". Create it or check permissions.", cur_template_dir);
		debug_print_1("Template directory is \"%s\"", cur_template_dir);
	}
	ctpp_init();
}
