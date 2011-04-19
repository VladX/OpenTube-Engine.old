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

/* Wrapper for FreeBSD */

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


static inline void kqueue_change (int fd, short filter)
{
	static struct kevent ev;
	
	ev.ident = fd;
	ev.filter = filter;
	ev.flags = EV_ADD | EV_ENABLE;
	ev.fflags = 0;
	ev.data = 0;
	ev.udata = NULL;
	
	kevent(kq, &ev, 1, NULL, 0, NULL);
}

inline void set_read_mask (int fd)
{
	kqueue_change(fd, EVFILT_READ);
}

inline void set_write_mask (int fd)
{
	kqueue_change(fd, EVFILT_WRITE);
}

inline void end_request(request_t * r)
{
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_NOPUSH, &disable, sizeof(disable)) == -1)
		perr("setsockopt(%d)", r->sock);
	
	http_cleanup(r);
}

void event_routine (void)
{
	const int srvfd = sockfd;
	const int enable = 1;
	const long timeout_sec = EPOLL_TIMEOUT / 1000L;
	const long timeout_nsec = (EPOLL_TIMEOUT % 1000L) * 1000000L;
	int n, i, fd;
	request_t * r;
	socklen_t client_name_len;
	struct timespec timeout;
	struct kevent e[config.prealloc_request_structures];
	struct sockaddr * addr;
	struct linger linger_opt;
	
	linger_opt.l_onoff = 1;
	linger_opt.l_linger = 0;
	
	event_startup(&addr, &client_name_len);
	
	kq = kqueue();
	
	if (kq == -1)
		peerr(0, "kqueue(): %d", -1);
	
	kqueue_change(srvfd, EVFILT_READ);
	
	for (;;)
	{
		event_iter();
		
		timeout.tv_sec = timeout_sec;
		timeout.tv_nsec = timeout_nsec;
		
		n = kevent(kq, NULL, 0, e, config.prealloc_request_structures, &timeout);
		
		for (i = 0; i < n; i++)
		{
			if (e[i].ident == srvfd)
			{
				#ifdef HAVE_ACCEPT4
				while ((fd = accept4(srvfd, addr, &client_name_len, SOCK_NONBLOCK)) != -1)
				{
				#else
				while ((fd = accept(srvfd, addr, &client_name_len)) != -1)
				{
					(void) fcntl(fd, F_SETFL, O_NONBLOCK);
				#endif
					debug_print_2("accept(): %d", fd);
					if (config.limit_req && limit_requests(addr))
					{
						debug_print_2("client has exceeded the allowable requests per second (rps) limit, request %d discarded", fd);
						setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
						socket_close(fd);
						break;
					}
					if (config.limit_sim_req && limit_sim_requests(addr, client_name_len, fd))
					{
						debug_print_2("client has exceeded the allowable simultaneous requests limit, request %d discarded", fd);
						setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
						socket_close(fd);
						break;
					}
					
					kqueue_change(fd, EVFILT_READ);
					
					if (* http_server_tcp_addr.str && setsockopt(fd, IPPROTO_TCP, TCP_NOPUSH, &enable, sizeof(enable)) == -1)
						perr("setsockopt(): %d", -1);
				}
				continue;
			}
			else if (e[i].filter == EVFILT_READ)
			{
				if (e[i].data == 0 && (e[i].flags & EV_EOF))
				{
					r = event_find_request(e[i].ident);
					
					if (r != NULL)
					{
						r->keepalive = false;
						end_request(r);
					}
					
					continue;
				}
				
				r = event_fetch_request(e[i].ident);
				
				if (http_serve_client(r))
					end_request(r);
			}
			else if (e[i].filter == EVFILT_WRITE)
			{
				if (e[i].flags & EV_EOF)
				{
					r = event_find_request(e[i].ident);
					
					if (r != NULL)
					{
						r->keepalive = false;
						end_request(r);
					}
					
					continue;
				}
				
				pthread_mutex_lock(wmutex);
				events_out_data(e[i].ident);
				pthread_mutex_unlock(wmutex);
			}
		}
	}
}

#endif
#endif
