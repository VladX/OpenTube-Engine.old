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

#ifndef _WIN32_UTILS_H
#define _WIN32_UTILS_H 1

#ifdef _WIN
#undef WINVER
#define WINVER 0x0501
#include <windows.h>
#include <ws2tcpip.h>
#include <glob.h>
#undef WINVER

struct iovec {
	void * iov_base;
	size_t iov_len;
};

int win32_glob (const char * pattern, int flags, void * error_cb, glob_t * pglob);

void win32_globfree (glob_t * pglob);

char * inet_ntop (int af, const void * src, char * dst, socklen_t cnt);

int inet_pton (int af, const char * src, void * dst);

ssize_t writev (int fd, const struct iovec * vector, int count);

ssize_t sendfile (int out_fd, int in_fd, off_t * offset, size_t count);

void win32_fatal_error (const char * msg);

LPCWSTR win32_utf8_to_utf16 (const char * src);

#endif
#endif
