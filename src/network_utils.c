/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "network_utils.h"
#include "common_functions.h"

#ifdef _WIN
 #include "win32_utils.h"
#endif

#if DEBUG_LEVEL > 2
static const char * addr2str (const int ai_family, struct sockaddr * addr)
{
	static char addr_str[48];
	
	memset(addr_str, 0, sizeof(addr_str));
	
	#if IPV6_SUPPORT
	if (ai_family == AF_INET6)
		inet_ntop(ai_family, &(((struct sockaddr_in6 *) addr)->sin6_addr), addr_str, sizeof(addr_str));
	#endif
	
	if (ai_family == AF_INET)
		inet_ntop(ai_family, &(((struct sockaddr_in *) addr)->sin_addr), addr_str, sizeof(addr_str));
	
	return addr_str;
}
#endif

size_t net_gethostaddr (char * name, unsigned short port, const int type, struct sockaddr * result_addr)
{
	struct addrinfo hints;
	struct addrinfo * res;
	int err;
	char port_str[8];
	
	memset(&hints, 0, sizeof(hints));
	int_to_str(port, port_str, 10);
	
	#if IPV6_SUPPORT
	hints.ai_family = type;
	#else
	hints.ai_family = AF_INET;
	#endif
	hints.ai_socktype = SOCK_STREAM;
	
	err = getaddrinfo(name, port_str, &hints, &res);
	
	if (err)
	{
		err("getaddrinfo(%s) failed: %s", name, gai_strerror(err));
		
		return 0;
	}
	
	debug_print_3("gethostaddr(): %s -> %s", name, addr2str(res->ai_family, res->ai_addr));
	
	memcpy(result_addr, res->ai_addr, res->ai_addrlen);
	
	freeaddrinfo(res);
	
	return res->ai_addrlen;
}
