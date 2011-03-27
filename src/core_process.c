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
	#ifndef _WIN
	const int len = strlen(procname);
	memset(procname, 0, len);
	memcpy(procname, newprocname, min(len, strlen(newprocname)));
	#endif
}

static void quit_worker (int prm)
{
	exit(0);
}

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
		#ifdef _WIN
		{
			perror("fopen()");
			exit(0);
		}
		#else
			peerr(0, "Can't create \"%s\"", config.pid);
		#endif
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
		if (p == 0)
		{
			f = NULL;
			goto from_beginning;
		}
		if (pid != p)
		{
			#ifdef _WIN
			HANDLE tmphdl = OpenProcess(SYNCHRONIZE, FALSE, (DWORD) p);
			if (tmphdl == NULL)
				goto from_beginning;
			else
			{
				CloseHandle(tmphdl);
				
				return false;
			}
			#else
			if (kill((pid_t) p, 0) == -1)
				goto from_beginning;
			else
				return false;
			#endif
		}
	}
	
	#ifdef _WIN
	Sleep(500);
	#endif
	
	return true;
}

pid_t spawn_worker (char * procname)
{
	unsigned char respawn_fails = 0;
	int status;
	time_t start_time;
	pid_t pid = 0;
	bool lock = create_pidfile();
	
	#ifdef HAVE_FORK_SYSCALL
	
	struct passwd * pwd;
	struct group * grp;
	
	setprocname(procname, PROG_NAME " (master)");
	
	if (!lock)
		eerr(1, "Master process is already running (\"%s\").", config.pid);
	
	for (;;)
	{
		start_time = time(NULL);
		pid = fork();
		
		if (pid < 0)
			peerr(1, "fork(): %d", pid);
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
		
		worker_pid = pid;
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
	
	#else
	#ifdef HAVE_CREATE_PROCESS_WITH_LOGONW
	
	if (!lock)
	{
		pid = getpid();
		
		if (signal(SIGINT, quit_worker) == SIG_ERR)
			err("can't handle signal %d", SIGINT);
		if (signal(SIGTERM, quit_worker) == SIG_ERR)
			err("can't handle signal %d", SIGTERM);
		
		return pid;
	}
	
	extern int sockfd;
	socket_close(sockfd);
	
	STARTUPINFO lpStartupInfo;
	PROCESS_INFORMATION lpProcessInfo;
	LPCWSTR user, group;
	
	user = win32_utf8_to_utf16(config.user);
	group = win32_utf8_to_utf16(config.group);
	
	LPWSTR cmdline = _wcsdup(GetCommandLineW());
	
	setprocname(procname, PROG_NAME " (master)");
	
	for (;;)
	{
		memset(&lpStartupInfo, 0, sizeof(STARTUPINFO));
		memset(&lpProcessInfo, 0, sizeof(PROCESS_INFORMATION));
		lpStartupInfo.cb = sizeof(STARTUPINFO);
		
		if (CreateProcessWithLogonW(user, group, WIN32_DEFAULT_PASWORD, 0, NULL, cmdline, 0, NULL, NULL, &lpStartupInfo, &lpProcessInfo) == 0)
			win32_fatal_error("CreateProcessWithLogonW()");
		
		start_time = time(NULL);
		
		worker_pid = (pid_t) GetProcessId(lpProcessInfo.hProcess);
		WaitForSingleObject(lpProcessInfo.hProcess, INFINITE);
		DWORD exit_code;
		GetExitCodeProcess(lpProcessInfo.hProcess, &exit_code);
		status = exit_code;
		CloseHandle(lpProcessInfo.hProcess);
		CloseHandle(lpProcessInfo.hThread);
		
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
	
	free((void *) user);
	free((void *) group);
	free((void *) cmdline);
	#else
	pid = getpid();
	#endif
	#endif
	
	return pid;
}
