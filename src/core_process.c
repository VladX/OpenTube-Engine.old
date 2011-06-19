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
#include "os_stat.h"
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "common_functions.h"
#include "core_server.h"
#include "core_process.h"
#include "win32_utils.h"
#ifdef HAVE_PRCTL_H
 #include <sys/prctl.h>
#endif

#ifndef _WIN
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
#endif

static void setprocname (char * procname, const char * newprocname)
{
	#ifdef _WIN
	(void) SetConsoleTitleA(newprocname);
	#else
	const int len = strlen(procname);
	memset(procname, 0, len);
	memcpy(procname, newprocname, min(len, strlen(newprocname)));
	#ifdef HAVE_PRCTL_SET_PROC_NAME
	(void) prctl(PR_SET_NAME, newprocname, 0, 0, 0);
	#endif
	#endif
}

#ifndef _WIN
static void quit_worker (int prm)
{
	exit(0);
}
#endif

#ifndef _WIN
bool kill_master_process (void)
{
	pid_t pid = 0;
	FILE * f = fopen(config.pid, "r");
	
	if (f == NULL)
		return false;
	
	if (fscanf(f, "%d", &pid) == 0)
	{
		fclose(f);
		remove_pidfile();
		
		return false;
	}
	
	if (kill(pid, SIGTERM) == -1)
	{
		fclose(f);
		peerr(-1, "Can't kill master process (%d)", pid);
	}
	
	return true;
}
#endif

void remove_pidfile (void)
{
	#ifdef _WIN
	FILE * f = fopen(config.pid, "w");
	if (f != NULL)
		fclose(f);
	#else
	(void) remove(config.pid);
	#endif
}

#ifndef _WIN
static bool create_pidfile (void)
{
	FILE * f;
	pid_t pid = getpid();
	
	f = fopen(config.pid, "r");
	
	from_beginning:
	
	if (f == NULL)
	{
		f = fopen(config.pid, "w");
		if (f == NULL)
			peerr(-1, "Can't create \"%s\"", config.pid);
		fprintf(f, "%d\n", (int) pid);
		fclose(f);
	}
	else
	{
		int p = 0;
		if (fscanf(f, "%d", &p) == 0)
		{
			fclose(f);
			f = NULL;
			goto from_beginning;
		}
		fclose(f);
		f = NULL;
		if (p == 0)
			goto from_beginning;
		if (pid != p)
		{
			if (kill((pid_t) p, 0) == -1)
				goto from_beginning;
			else
				return false;
		}
	}
	
	return true;
}
#endif

pid_t spawn_worker (char * procname)
{
	pid_t pid = 0;
	#ifdef HAVE_FORK_SYSCALL
	unsigned char respawn_fails = 0;
	int status;
	time_t start_time;
	bool lock = create_pidfile();
	
	struct passwd * pwd;
	struct group * grp;
	
	setprocname(procname, PROG_NAME " (master)");
	
	if (!lock)
		eerr(-1, "Master process is already running (\"%s\").", config.pid);
	
	for (;;)
	{
		start_time = time(NULL);
		pid = fork();
		
		if (pid < 0)
			peerr(-1, "fork(): %d", pid);
		if (pid == 0)
		{
			pid = getpid();
			pwd = getpwnam(config.user);
			if (!pwd)
				eerr(-1, "can't find user with name %s", config.user);
			setuid(pwd->pw_uid);
			grp = getgrnam(config.group);
			if (!grp)
				grp = getgrnam(WORKER_GROUP_DEFAULT);
			if (!grp)
				setgid(pwd->pw_gid);
			else
				setgid(grp->gr_gid);
			
			setprocname(procname, PROG_NAME " (worker)");
			
			if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
				err("can't ignore signal %d", SIGPIPE);
			if (signal(SIGINT, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGINT);
			if (signal(SIGTERM, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGTERM);
			if (signal(SIGQUIT, quit_worker) == SIG_ERR)
				err("can't handle signal %d", SIGQUIT);
			
			#ifdef HAVE_PRCTL_H
			(void) prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
			#endif
			
			break;
		}
		
		worker_pid = pid;
		waitpid(pid, &status, 0);
		
		if (WIFEXITED(status))
			quit(WEXITSTATUS(status));
		else
		{
			status = WTERMSIG(status);
			
			if (time(NULL) - start_time <= 1)
				respawn_fails++;
			else
				respawn_fails = 0;
			
			if (respawn_fails > 10)
				eerr(-1, "worker crashed with status %d (%s)", status, statustomsg(status));
		}
		
		log_msg("worker process terminated with status %d (%s), respawning...", status, statustomsg(status));
	}
	
	#else
	setprocname(procname, PROG_NAME);
	pid = getpid();
	#endif
	
	return pid;
}
