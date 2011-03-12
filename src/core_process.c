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

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "common_functions.h"
#include "core_server.h"
#include "core_process.h"
#include "win32_utils.h"


static const char * statustomsg (int status)
{
	switch (status)
	{
		case 2:
			return "INT";
			break;
		case 9:
			return "KILL";
			break;
		case 11:
			return "SEGV - segmentation fault";
			break;
		case 15:
			return "TERM";
			break;
	}
	
	return NULL;
}

static void setprocname (char * procname, const char * newprocname)
{
	const int len = strlen(procname);
	memset(procname, 0, len);
	memcpy(procname, newprocname, min(len, strlen(newprocname)));
}

static void quit_worker (int prm)
{
	exit(0);
}

pid_t spawn_worker (char * procname)
{
	#ifdef _WIN
	pid_t pid = getpid();
	#else
	unsigned char respawn_fails = 0;
	int status;
	time_t start_time;
	pid_t pid = 0;
	struct passwd * pwd;
	struct group * grp;
	struct stat stat_buf;
	
	setprocname(procname, PROG_NAME " (master)");
	
	for (;;)
	{
		start_time = time(NULL);
		pid = fork();
		
		if (pid < 0)
			peerr(9, "fork(): %d", pid);
		if (pid == 0)
		{
			pid = getpid();
			pwd = getpwnam(config.user);
			if (!pwd)
				eerr(0, "can't find user with name %s", config.user);
			setuid(pwd->pw_uid);
			grp = getgrnam(config.group);
			if (!grp)
				grp = getgrnam(WORKER_GROUP_DEFAULT);
			if (!grp)
				setgid(pwd->pw_gid);
			else
				setgid(grp->gr_gid);
			
			if (stat(config.temp_dir, &stat_buf) == -1)
				peerr(0, "Access to directory \"%s\"", config.temp_dir);
			
			if (stat_buf.st_uid != pwd->pw_uid)
				eerr(0, "Owner of the directory \"%s\" is not \"%s\".", config.temp_dir, config.user);
			
			setprocname(procname, PROG_NAME " (worker)");
			
			if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
				err("can't ignore signal %d", SIGPIPE);
			if (signal(SIGINT, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGINT);
			if (signal(SIGTERM, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGTERM);
			if (signal(SIGQUIT, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGQUIT);
			
			break;
		}
		
		waitpid(pid, &status, 0);
		
		if (status == 0)
			quit(0);
		else
		{
			if (time(NULL) - start_time <= 1)
				respawn_fails++;
			else
				respawn_fails = 0;
			
			if (respawn_fails > 10)
				eerr(12, "worker crashed with status %d (%s)", status, statustomsg(status));
		}
		
		debug_print_1("worker process terminated with status %d (%s), respawning...", status, statustomsg(status));
	}
	#endif
	
	return pid;
}
