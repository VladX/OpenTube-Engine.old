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
#include "core_process.h"
#include "core_server.h"
#include "core_config.h"
#include "win32_service.h"

#define PROG_DESCR PROG_NAME " " PROG_VERSION ".\n\n    "
#define COPYRIGHT "Copyright (C) 2011 VladX (http://vladx.net/); Bugs to <vvladxx@gmail.com>."

#ifndef _WIN
static void detach_process (void)
{
	pid_t pid = fork();
	if (pid != 0)
		exit(0);
	(void) setsid();
	
	logger_set_file_output();
	if (freopen("/dev/null", "w", stdout) == NULL || freopen("/dev/null", "w", stderr) == NULL)
		eerr(-1, "freopen(%s, %s) failed", "/dev/null", "w");
	fclose(stdin);
}
#endif

static void parse_args (char ** args, int n)
{
	char * config_path = NULL;
	
	logger_set_console_output();
	
	#ifdef _WIN
	if (n > 1 && (strcmp("-c", args[1]) != 0 || n < 3))
	{
		eer_f(stderr, PROG_DESCR "Usage: %s [-c /path/to/configuration/file]\n\n" COPYRIGHT "\n", args[0]);
		exit(-1);
	}
	
	if (n >= 3)
		config_path = args[2];
	#else
	uint i;
	bool detach = false;
	bool kill_master = false;
	
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
		else if (strcmp(args[i], "--kill") == 0 || strcmp(args[i], "-k") == 0)
		{
			kill_master = true;
			continue;
		}
		
		err_f(stderr, PROG_DESCR "Usage: %s [--detach] [-c /path/to/configuration/file]\n\n" COPYRIGHT "\n", args[0]);
		exit(-1);
	}
	
	#if DEBUG_LEVEL
	logger_set_console_output();
	#else
	logger_set_both_output();
	#endif
	
	if (kill_master)
		detach = false;
	
	if (detach)
		detach_process();
	#endif
	
	if (config_path)
		load_config(config_path);
	else
		load_config(CONFIG_PATH);
	
	#ifndef _WIN
	if (kill_master)
	{
		if (!kill_master_process())
			eerr(-1, "%s", "Master process is not running");
		exit(0);
	}
	#endif
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
