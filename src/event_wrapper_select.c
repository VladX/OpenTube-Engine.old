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

#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "common_functions.h"
#include "core_http.h"
#include "event_wrapper.h"

#ifndef TCP_CORK
 #ifdef _BSD
  #define TCP_CORK TCP_NOPUSH
 #endif
#endif

#ifndef HAVE_EPOLL
#ifdef HAVE_SELECT


extern pthread_mutex_t wmutex[1];
extern int sockfd;

static pthread_mutex_t mutex[1];
static fd_set rfds;
static fd_set wfds;
static int socklist[SELECT_MAX_CONNECTIONS];
static uchar sockmask[SELECT_MAX_CONNECTIONS];
static uint socklist_len = 0;
static int maxfd_plus_one = 0;

#define SELECT_READ 1
#define SELECT_WRITE 2


#ifdef _WIN
 #define recalc_maxfd_plus_one()
#else
static inline void recalc_maxfd_plus_one (void)
{
	static uint i;
	
	maxfd_plus_one = sockfd + 1;
	
	for (i = 0; i < socklist_len; i++)
		if (socklist[i] >= maxfd_plus_one)
			maxfd_plus_one = socklist[i] + 1;
}
#endif

static inline void _add (int fd)
{
	pthread_mutex_lock(mutex);
	if (socklist_len >= SELECT_MAX_CONNECTIONS)
	{
		pthread_mutex_unlock(mutex);
		return;
	}
	socklist[socklist_len] = fd;
	sockmask[socklist_len] = SELECT_READ;
	socklist_len++;
	recalc_maxfd_plus_one();
	pthread_mutex_unlock(mutex);
}

inline void _select_socket_del_from_event_list (int fd)
{
	static uint i;
	
	pthread_mutex_lock(mutex);
	
	for (i = 0; i < socklist_len; i++)
		if (socklist[i] == fd)
		{
			socklist_len--;
			socklist[i] = socklist[socklist_len];
			sockmask[i] = sockmask[socklist_len];
			break;
		}
	
	recalc_maxfd_plus_one();
	pthread_mutex_unlock(mutex);
}

static inline void set_select_event_mask (int fd, uchar mask)
{
	static uint i;
	
	pthread_mutex_lock(mutex);
	
	for (i = 0; i < socklist_len; i++)
		if (socklist[i] == fd)
		{
			sockmask[i] = mask;
			break;
		}
	
	pthread_mutex_unlock(mutex);
}

void set_read_mask (int fd)
{
	set_select_event_mask(fd, SELECT_READ);
}

void set_write_mask (int fd)
{
	set_select_event_mask(fd, SELECT_WRITE);
}

inline void end_request(request_t * r)
{
	#ifndef _WIN
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1)
		perr("setsockopt(%d)", r->sock);
	#endif
	
	http_cleanup(r);
}

void event_routine (void)
{
	const int maxevents = MAX_EVENTS;
	const int srvfd = sockfd;
	const int enable = 1;
	const long timeout_sec = EPOLL_TIMEOUT / 1000L;
	const long timeout_usec = (EPOLL_TIMEOUT % 1000L) * 1000L;
	int n, i, it, fd;
	socklen_t client_name_len;
	request_t * request[maxevents];
	struct sockaddr * addr;
	struct linger linger_opt;
	struct timeval tv;
	
	pthread_mutex_init(mutex, NULL);
	
	linger_opt.l_onoff = 1;
	linger_opt.l_linger = 0;
	
	event_startup(maxevents, request, &addr, &client_name_len);
	
	maxfd_plus_one = srvfd + 1;
	
	socklist_len = 0;
	
	for (;;)
	{
		event_iter(request);
		
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(srvfd, &rfds);
		
		pthread_mutex_lock(mutex);
		for (i = 0; i < socklist_len; i++)
		{
			#ifdef _WIN
			if (recv(socklist[i], NULL, 0, MSG_PEEK) == -1 && errno != EAGAIN)
			#else
			if (fcntl(socklist[i], F_GETFL) == -1)
			#endif
			{
				socklist_len--;
				socklist[i] = socklist[socklist_len];
				sockmask[i] = sockmask[socklist_len];
				recalc_maxfd_plus_one();
				i--;
				continue;
			}
			if (sockmask[i] == SELECT_READ)
				FD_SET(socklist[i], &rfds);
			else
				FD_SET(socklist[i], &wfds);
		}
		pthread_mutex_unlock(mutex);
		
		tv.tv_sec = timeout_sec;
		tv.tv_usec = timeout_usec;
		
		n = select(maxfd_plus_one, &rfds, &wfds, NULL, &tv);
		
		if (n <= 0)
			continue;
		
		if (FD_ISSET(srvfd, &rfds))
		{
				#ifdef HAVE_ACCEPT4
				while ((fd = accept4(srvfd, addr, &client_name_len, SOCK_NONBLOCK)) != -1)
				{
				#else
				while ((fd = accept(srvfd, addr, &client_name_len)) != -1)
				{
					#ifdef _WIN
					ioctlsocket(fd, FIONBIO, (void *) &enable);
					#else
					(void) fcntl(fd, F_SETFL, O_NONBLOCK);
					#endif
				#endif
					debug_print_2("accept(): %d", fd);
					if (config.limit_req && limit_requests(addr))
					{
						debug_print_2("client has exceeded the allowable requests per second (rps) limit, request %d discarded", fd);
						setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &linger_opt, sizeof(linger_opt));
						socket_close(fd);
						break;
					}
					if (config.limit_sim_req && limit_sim_requests(addr, client_name_len, fd))
					{
						debug_print_2("client has exceeded the allowable simultaneous requests limit, request %d discarded", fd);
						setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *) &linger_opt, sizeof(linger_opt));
						socket_close(fd);
						break;
					}
					
					_add(fd);
					
					#ifndef _WIN
					if (* http_server_tcp_addr.str && setsockopt(fd, IPPROTO_TCP, TCP_CORK, &enable, sizeof(enable)) == -1)
						perr("setsockopt(): %d", -1);
					#endif
				}
		}
		
		for (i = 0; i < socklist_len; i++)
		{
			pthread_mutex_lock(mutex);
			fd = socklist[i];
			pthread_mutex_unlock(mutex);
			
			if (FD_ISSET(fd, &rfds))
			{
				pthread_mutex_lock(wmutex);
				
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == socklist[i])
						goto _h_req;
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == -1)
						goto _h_req;
				pthread_mutex_unlock(wmutex);
				continue;
				_h_req:
				
				request[it]->sock = fd;
				
				pthread_mutex_unlock(wmutex);
				
				if (http_serve_client(request[it]))
					end_request(request[it]);
			}
			else if (FD_ISSET(socklist[i], &wfds))
			{
				pthread_mutex_lock(wmutex);
				events_out_data(maxevents, fd, request);
				pthread_mutex_unlock(wmutex);
			}
		}
	}
}

#endif
#endif
