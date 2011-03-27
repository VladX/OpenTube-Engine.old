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

/* This is a simple wrapper for systems, that doesn't support epoll_create() system call, but support select() */

#include "common_functions.h"

#ifndef HAVE_EPOLL
#ifdef HAVE_KQUEUE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/event.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "core_http.h"
#include "event_wrapper.h"


extern pthread_mutex_t wmutex[1];
extern int sockfd;

static int kq;


inline void end_request(request_t * r)
{
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_NOPUSH, &disable, sizeof(disable)) == -1)
		perr("setsockopt(%d)", r->sock);
	
	http_cleanup(r);
}

void event_routine (void)
{
	kq = kqueue();
	
	if (kq == -1)
		peerr(0, "kqueue()");
	
	
}

#endif
#endif
