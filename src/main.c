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
#include "core_server.h"
#include "core_config.h"

#define COPYRIGHT "Copyright (C) 2011 VladX (http://vladx.net/); Bugs to <vvladxx@gmail.com>"


void parse_args (char ** args, int n)
{
	if (n > 1 && (strcmp("-c", args[1]) != 0 || n < 3))
		eerr(1, "Usage: %s [-c /path/to/configuration/file]\n\n" COPYRIGHT, args[0]);
	
	if (n >= 3)
		load_config(args[2]);
	else
		load_config(CONFIG_PATH);
}

int main (int argc, char ** argv)
{
	parse_args(argv, argc);
	init(* argv);
	
	return 0;
}
