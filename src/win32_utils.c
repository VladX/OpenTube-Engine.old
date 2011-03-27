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
#ifdef _WIN
#include <win32_utils.h>
#include <io.h>

/* Wrapper functions for Windows */

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

ssize_t sendfile (int out_fd, int in_fd, off_t * offset, size_t count)
{
	char * buf[READ_SIZE];
	int r, c;
	ssize_t total = 0;
	
	if (offset != NULL)
		lseek(in_fd, * offset, SEEK_SET);
	
	for (;;)
	{
		c = count - total;
		c = max(0, c);
		c = min(READ_SIZE, c);
		r = read(in_fd, buf, c);
		if (r == -1)
		{
			if (offset != NULL)
				lseek(in_fd, 0, SEEK_SET);
			return -1;
		}
		r = send(out_fd, (const char *) buf, r, 0);
		if (r == -1)
		{
			if (offset != NULL)
				lseek(in_fd, 0, SEEK_SET);
			return -1;
		}
		total += r;
		if (total >= count)
		{
			if (offset != NULL)
				lseek(in_fd, 0, SEEK_SET);
			* offset += total;
			return total;
		}
	}
	
	return 0;
}

void win32_fatal_error (const char * msg)
{
	LPVOID lpvMessageBuffer;
	
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  (LPTSTR) &lpvMessageBuffer, 0, NULL);
	
	eerr(0, "%s: %s (code: %d)", msg, (char *) lpvMessageBuffer, (int) GetLastError());
	
	ExitProcess(GetLastError());
}

LPCWSTR win32_utf8_to_utf16 (const char * src)
{
	uint l = (uint) strlen(src);
	uint len = (uint) MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) src, l, NULL, 0);
	wchar_t * dst = malloc(sizeof(wchar_t) * (len + 1));
	(void) MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) src, l, (LPWSTR) dst, len);
	dst[len] = 0;
	
	return (LPCWSTR) dst;
}

#endif
