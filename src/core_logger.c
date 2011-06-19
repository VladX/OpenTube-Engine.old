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
#include "win32_utils.h"
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#ifdef HAVE_SYSLOG_H
 #include <syslog.h>
#endif

#ifndef LOG_EMERG
 #define LOG_EMERG 0
#endif
#ifndef LOG_ERR
 #define LOG_ERR 3
#endif

enum logger_output
{
	O_CONSOLE,
	O_FILE,
	O_BOTH,
	O_UNKNOWN
};

enum logger_level
{
	L_NOTICE,
	L_ERROR,
	L_CRITICAL
};

static const char * logger_level_strings[] = {"Error", "Critical"};
static enum logger_output output = O_UNKNOWN;
static FILE * log_file = NULL;
static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

void logger_set_console_output (void)
{
	output = O_CONSOLE;
}

void logger_set_file_output (void)
{
	output = O_FILE;
}

void logger_set_both_output (void)
{
	output = O_BOTH;
}

static void v_logger_syslog (int level, const char * fmt, va_list ap)
{
	#ifdef HAVE_SYSLOG_H
	openlog(PROG_NAME, LOG_PID, LOG_DAEMON);
	vsyslog(level, fmt, ap);
	closelog();
	#endif
}

static void logger_syslog (int level, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_syslog(level, fmt, ap);
	va_end(ap);
}

static void print_level_str (FILE * out, enum logger_level level, bool colorize)
{
	const char * level_str;
	#ifdef _WIN
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	#endif
	
	switch (level)
	{
		case L_ERROR:
			if (colorize)
			{
				#ifdef _WIN
				SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED);
				#else
				fprintf(out, "\x1b[0;37;41m");
				#endif
			}
			level_str = logger_level_strings[0];
			break;
		case L_CRITICAL:
			if (colorize)
			{
				#ifdef _WIN
				SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_RED);
				#else
				fprintf(out, "\x1b[1;37;41m");
				#endif
			}
			level_str = logger_level_strings[1];
			break;
		default:
			return;
	}
	
	#ifdef _WIN
	fprintf(out, "[%s]", level_str);
	if (colorize)
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	fprintf(out, " ");
	#else
	if (colorize)
		fprintf(out, "[%s]\x1b[0;0;0m ", level_str);
	else
		fprintf(out, "[%s] ", level_str);
	#endif
}

static void print_sys_error (FILE * out)
{
	#ifdef _WIN
	DWORD err = GetLastError();
	if (!err)
		err = WSAGetLastError();
	if (err)
	{
		char * errstr = win32_strerror(err);
		if (out == stdout || out == stderr)
			(void) CharToOemBuffA(errstr, errstr, strlen(errstr));
		fprintf(out, ": %s", errstr);
		LocalFree(errstr);
	}
	#else
	if (errno)
		fprintf(out, ": %s", strerror(errno));
	#endif
}

static void _logger_log_console (bool sys_error, enum logger_level level, const char * fmt, va_list ap)
{
	FILE * f;
	
	pthread_mutex_lock(mutex);
	f = (level == L_NOTICE) ? stdout : stderr;
	print_level_str(f, level, true);
	vfprintf(f, fmt, ap);
	if (sys_error)
		print_sys_error(f);
	fprintf(f, "\n");
	pthread_mutex_unlock(mutex);
}

static void v_logger_log_console (const char * fmt, va_list ap)
{
	_logger_log_console(false, L_NOTICE, fmt, ap);
}

void logger_log_console (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_console(fmt, ap);
	va_end(ap);
}

static void v_logger_log_console_error (const char * fmt, va_list ap)
{
	_logger_log_console(false, L_ERROR, fmt, ap);
}

void logger_log_console_error (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_console_error(fmt, ap);
	va_end(ap);
}

static void v_logger_log_console_critical (int exit_code, const char * fmt, va_list ap)
{
	_logger_log_console(false, L_CRITICAL, fmt, ap);
}

void logger_log_console_critical (int exit_code, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_console_critical(exit_code, fmt, ap);
	va_end(ap);
	exit(exit_code);
}

static void v_logger_log_console_perror (const char * fmt, va_list ap)
{
	_logger_log_console(true, L_ERROR, fmt, ap);
}

void logger_log_console_perror (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_console_perror(fmt, ap);
	va_end(ap);
}

static void v_logger_log_console_pcritical (int exit_code, const char * fmt, va_list ap)
{
	_logger_log_console(true, L_CRITICAL, fmt, ap);
}

void logger_log_console_pcritical (int exit_code, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_console_pcritical(exit_code, fmt, ap);
	va_end(ap);
	exit(exit_code);
}

static void _logger_log_file (bool sys_error, enum logger_level level, const char * fmt, va_list ap)
{
	pthread_mutex_lock(mutex);
	if (log_file == NULL)
	{
		v_logger_syslog(LOG_ERR, fmt, ap);
		pthread_mutex_unlock(mutex);
		return;
	}
	
	_BEGIN_LOCAL_SECTION_
	const time_t curtime = time(NULL);
	
	fprintf(log_file, "%.*s ", 24, ctime(&curtime));
	if (* http_server_unix_addr.str)
		fprintf(log_file, "%s ", http_server_unix_addr.str);
	else if (* http_server_tcp_addr.str)
		fprintf(log_file, "%s:%d ", http_server_tcp_addr.str, http_port);
	print_level_str(log_file, level, false);
	vfprintf(log_file, fmt, ap);
	if (sys_error)
		print_sys_error(log_file);
	fprintf(log_file, "\n");
	_END_LOCAL_SECTION_
	pthread_mutex_unlock(mutex);
}

static void v_logger_log_file (const char * fmt, va_list ap)
{
	_logger_log_file(false, L_NOTICE, fmt, ap);
}

void logger_log_file (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_file(fmt, ap);
	va_end(ap);
}

static void v_logger_log_file_error (const char * fmt, va_list ap)
{
	_logger_log_file(false, L_ERROR, fmt, ap);
}

void logger_log_file_error (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_file_error(fmt, ap);
	va_end(ap);
}

static void v_logger_log_file_critical (int exit_code, const char * fmt, va_list ap)
{
	_logger_log_file(false, L_CRITICAL, fmt, ap);
}

void logger_log_file_critical (int exit_code, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_file_critical(exit_code, fmt, ap);
	va_end(ap);
	exit(exit_code);
}

static void v_logger_log_file_perror (const char * fmt, va_list ap)
{
	_logger_log_file(true, L_ERROR, fmt, ap);
}

void logger_log_file_perror (const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_file_perror(fmt, ap);
	va_end(ap);
}

static void v_logger_log_file_pcritical (int exit_code, const char * fmt, va_list ap)
{
	_logger_log_file(true, L_CRITICAL, fmt, ap);
}

void logger_log_file_pcritical (int exit_code, const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	v_logger_log_file_pcritical(exit_code, fmt, ap);
	va_end(ap);
	exit(exit_code);
}

void logger_log (const char * fmt, ...)
{
	va_list ap1;
	va_list ap2;
	va_start(ap1, fmt);
	va_start(ap2, fmt);
	
	switch (output)
	{
		case O_CONSOLE:
			v_logger_log_console(fmt, ap1);
			break;
		case O_FILE:
			v_logger_log_file(fmt, ap1);
			break;
		case O_BOTH:
			v_logger_log_console(fmt, ap1);
			v_logger_log_file(fmt, ap2);
			break;
		default:
			break;
	}
	
	va_end(ap1);
	va_end(ap2);
}

void logger_log_error (const char * fmt, ...)
{
	va_list ap1;
	va_list ap2;
	va_start(ap1, fmt);
	va_start(ap2, fmt);
	
	switch (output)
	{
		case O_CONSOLE:
			v_logger_log_console_error(fmt, ap1);
			break;
		case O_FILE:
			v_logger_log_file_error(fmt, ap1);
			break;
		case O_BOTH:
			v_logger_log_console_error(fmt, ap1);
			v_logger_log_file_error(fmt, ap2);
			break;
		default:
			break;
	}
	
	va_end(ap1);
	va_end(ap2);
}

void logger_log_perror (const char * fmt, ...)
{
	va_list ap1;
	va_list ap2;
	va_start(ap1, fmt);
	va_start(ap2, fmt);
	
	switch (output)
	{
		case O_CONSOLE:
			v_logger_log_console_perror(fmt, ap1);
			break;
		case O_FILE:
			v_logger_log_file_perror(fmt, ap1);
			break;
		case O_BOTH:
			v_logger_log_console_perror(fmt, ap1);
			v_logger_log_file_perror(fmt, ap2);
			break;
		default:
			break;
	}
	
	va_end(ap1);
	va_end(ap2);
}

void logger_log_critical (int exit_code, const char * fmt, ...)
{
	va_list ap1;
	va_list ap2;
	va_start(ap1, fmt);
	va_start(ap2, fmt);
	
	switch (output)
	{
		case O_CONSOLE:
			v_logger_log_console_critical(exit_code, fmt, ap1);
			break;
		case O_FILE:
			v_logger_log_file_critical(exit_code, fmt, ap1);
			break;
		case O_BOTH:
			v_logger_log_console_critical(exit_code, fmt, ap1);
			v_logger_log_file_critical(exit_code, fmt, ap2);
			break;
		default:
			break;
	}
	
	va_end(ap1);
	va_end(ap2);
	exit(exit_code);
}

void logger_log_pcritical (int exit_code, const char * fmt, ...)
{
	va_list ap1;
	va_list ap2;
	va_start(ap1, fmt);
	va_start(ap2, fmt);
	
	switch (output)
	{
		case O_CONSOLE:
			v_logger_log_console_pcritical(exit_code, fmt, ap1);
			break;
		case O_FILE:
			v_logger_log_file_pcritical(exit_code, fmt, ap1);
			break;
		case O_BOTH:
			v_logger_log_console_pcritical(exit_code, fmt, ap1);
			v_logger_log_file_pcritical(exit_code, fmt, ap2);
			break;
		default:
			break;
	}
	
	va_end(ap1);
	va_end(ap2);
	exit(exit_code);
}

void logger_init (void)
{
	if (output == O_UNKNOWN)
		logger_set_both_output();
	log_file = fopen(config.log, "ab");
	if (log_file == NULL)
	{
		logger_syslog(LOG_EMERG, "Can't open log file \"%s\" for writing.", config.log);
		exit(-1);
	}
}
