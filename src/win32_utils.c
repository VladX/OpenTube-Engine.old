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

#define WINVER 0x0501
#include "common_functions.h"
#ifdef _WIN
#include "win32_utils.h"
#include <io.h>
#include <glob.h>

/* Wrapper functions for Windows */

/* Very simple version of POSIX glob() for Windows */
int win32_glob (const char * pattern, int flags, void * error_cb, glob_t * pglob)
{
	pglob->gl_pathc = 0;
	pglob->gl_pathv = NULL;
	
	_BEGIN_LOCAL_SECTION_
	WIN32_FIND_DATAA data;
	
	HANDLE h = FindFirstFileA(pattern, &data);
	
	if (h == INVALID_HANDLE_VALUE && io_errno == ERROR_FILE_NOT_FOUND)
		return 0;
	if (h == INVALID_HANDLE_VALUE)
		return -1;
	
	_BEGIN_LOCAL_SECTION_
	char * p = (char *) pattern + (strlen(pattern) - 1);
	char * basedir = NULL;
	uint basedir_len = 0, len;
	
	while (* p != '/' && p != pattern)
		p--;
	
	if (p != pattern)
	{
		p++;
		basedir = malloc((p - pattern) + 1);
		memcpy(basedir, pattern, p - pattern);
		basedir[p - pattern] = '\0';
		basedir_len = p - pattern;
	}
	
	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		pglob->gl_pathv = NULL;
		pglob->gl_pathc = 0;
	}
	else
	{
		pglob->gl_pathv = malloc(sizeof(char *));
		len = strlen(data.cFileName);
		pglob->gl_pathv[0] = malloc(basedir_len + len + 1);
		memcpy(pglob->gl_pathv[0], basedir, basedir_len);
		(pglob->gl_pathv[0])[basedir_len] = '\0';
		strcat(pglob->gl_pathv[0], data.cFileName);
		pglob->gl_pathc = 1;
	}
	
	while (FindNextFileA(h, &data) != 0)
	{
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		pglob->gl_pathv = realloc(pglob->gl_pathv, sizeof(char *) * (pglob->gl_pathc + 1));
		len = strlen(data.cFileName);
		pglob->gl_pathv[pglob->gl_pathc] = malloc(basedir_len + len + 1);
		memcpy(pglob->gl_pathv[pglob->gl_pathc], basedir, basedir_len);
		(pglob->gl_pathv[pglob->gl_pathc])[basedir_len] = '\0';
		strcat(pglob->gl_pathv[pglob->gl_pathc], data.cFileName);
		pglob->gl_pathc++;
	}
	
	if (basedir)
		free(basedir);
	
	FindClose(h);
	
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
	
	return 0;
}

void win32_globfree (glob_t * pglob)
{
	uint i;
	
	for (i = 0; i < pglob->gl_pathc; i++)
	{
		free(pglob->gl_pathv[i]);
		pglob->gl_pathv[i] = NULL;
	}
	
	free(pglob->gl_pathv);
}

#ifndef _MSVC_
char * inet_ntop (int af, const void * src, char * dst, socklen_t cnt)
{
	if (af == AF_INET)
	{
		struct sockaddr_in in;
		memset(&in, 0, sizeof(in));
		in.sin_family = AF_INET;
		memcpy(&(in.sin_addr), src, sizeof(struct in_addr));
		getnameinfo((struct sockaddr *) &in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
		
		return dst;
	}
	else if (af == AF_INET6)
	{
		struct sockaddr_in6 in;
		memset(&in, 0, sizeof(in));
		in.sin6_family = AF_INET6;
		memcpy(&(in.sin6_addr), src, sizeof(struct in_addr6));
		getnameinfo((struct sockaddr *) &in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
		
		return dst;
	}
	
	return NULL;
}

int inet_pton (int af, const char * src, void * dst)
{
	struct addrinfo hints, * res, * ressave;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = af;
	
	if (getaddrinfo(src, NULL, &hints, &res) != 0)
		return -1;
	
	ressave = res;
	
	while (res)
	{
		memcpy(dst, res->ai_addr, res->ai_addrlen);
		res = res->ai_next;
	}
	
	freeaddrinfo(ressave);
	
	return 0;
}
#endif

ssize_t writev (int fd, const struct iovec * vector, int count)
{
	unsigned int i;
	struct iovec * v;
	ssize_t n = 0;
	int r;
	
	for (i = 0; i < count; i++)
	{
		v = (struct iovec *) &(vector[i]);
		r = send(fd, v->iov_base, v->iov_len, 0);
		if (r == -1)
			return -1;
		n += r;
	}
	
	return n;
}

void win32_transmit_complete_cb (request_t * r, BOOLEAN timeout)
{
	if (timeout || r->temp.WaitHandle == NULL)
		return;
	
	close(r->temp.TransmitFileHandle);
	r->temp.TransmitFileHandle = -1;
	CloseHandle(r->temp.EventHandle);
	r->temp.EventHandle = NULL;
	
	end_request(r);
	
	UnregisterWait(r->temp.WaitHandle);
	r->temp.WaitHandle = NULL;
}

/* The caller must free returned buffer with LocalFree() after dealing with it */
char * win32_strerror (DWORD err)
{
	void * lpvMessageBuffer;
	
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  (void *) &lpvMessageBuffer, 0, NULL);
	
	return lpvMessageBuffer;
}

void win32_fatal_error (const char * msg)
{
	char * errstr = win32_strerror(GetLastError());
	
	if (* msg)
		printf("%s: %s (code: %lu)", msg, errstr, (ulong) GetLastError());
	else
		printf("%s (code: %lu)", errstr, (ulong) GetLastError());
	
	LocalFree(errstr);
	ExitProcess(GetLastError());
}

LPWSTR win32_utf8_to_utf16 (const char * src)
{
	uint l = (uint) strlen(src);
	uint len = (uint) MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) src, l, NULL, 0);
	wchar_t * dst = malloc(sizeof(wchar_t) * (len + 1));
	(void) MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) src, l, (LPWSTR) dst, len);
	dst[len] = 0;
	
	return (LPWSTR) dst;
}

#endif
