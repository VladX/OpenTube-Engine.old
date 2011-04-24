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
#include "win32_service.h"

#define COPYRIGHT "Copyright (C) 2011 VladX (http://vladx.net/); Bugs to <vvladxx@gmail.com>"

#ifndef _WIN
static void detach_process (void)
{
	pid_t pid = fork();
	if (pid != 0)
		exit(0);
	(void) setsid();
	
	logger_set_file_output();
}
#endif

static void parse_args (char ** args, int n)
{
	char * config_path = NULL;
	#ifdef _WIN
	
	if (n > 1 && (strcmp("-c", args[1]) != 0 || n < 3))
		eerr(1, "Usage: %s [-c /path/to/configuration/file]\n\n" COPYRIGHT, args[0]);
	
	if (n >= 3)
		config_path = args[2];
	#else
	uint i;
	bool detach = false;
	
	for (i = 1; i < n; i++)
	{
		if (strcmp(args[i], "-c") == 0)
		{
			i++;
			if (i < n)
			{
				config_path = args[i];
				continue;
			}
		}
		else if (strcmp(args[i], "--detach") == 0 || strcmp(args[i], "-d") == 0)
		{
			detach = true;
			continue;
		}
		
		err_f(stderr, "Usage: %s [--detach] [-c /path/to/configuration/file]\n\n" COPYRIGHT, args[0]);
		exit(1);
	}
	
	#if DEBUG_LEVEL
	logger_set_console_output();
	#else
	logger_set_both_output();
	#endif
	
	if (detach)
		detach_process();
	#endif
	
	if (config_path)
		load_config(config_path);
	else
		load_config(CONFIG_PATH);
}

int main (int argc, char ** argv)
{
	parse_args(argv, argc);
	logger_init();
	#if DEBUG_LEVEL
	#ifdef _WIN
	win32_service_init();
	#endif
	#endif
	init(* argv);
	
	return 0;
}
