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

#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H 1

#include <stdarg.h>

void logger_set_console_output (void);

void logger_set_file_output (void);

void logger_set_both_output (void);

void logger_log_console (const char * fmt, ...);

void logger_log_console_error (const char * fmt, ...);

void logger_log_console_perror (const char * fmt, ...);

void logger_log_console_critical (int exit_code, const char * fmt, ...);

void logger_log_console_pcritical (int exit_code, const char * fmt, ...);

void logger_log_file (const char * fmt, ...);

void logger_log_file_error (const char * fmt, ...);

void logger_log_file_perror (const char * fmt, ...);

void logger_log_file_critical (int exit_code, const char * fmt, ...);

void logger_log_file_pcritical (int exit_code, const char * fmt, ...);

void logger_log (const char * fmt, ...);

void logger_log_error (const char * fmt, ...);

void logger_log_perror (const char * fmt, ...);

void logger_log_critical (int exit_code, const char * fmt, ...);

void logger_log_pcritical (int exit_code, const char * fmt, ...);

void logger_init (void);

#endif
