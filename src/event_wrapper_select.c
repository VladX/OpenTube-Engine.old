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
#include "core_server.h"

#ifndef HAVE_EPOLL
#ifndef HAVE_KQUEUE
#ifdef HAVE_SELECT

#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#ifndef _MSC_VER
 #include <sys/time.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "core_http.h"
#include "event_wrapper.h"

#ifndef TCP_CORK
 #ifdef _BSD
  #define TCP_CORK TCP_NOPUSH
 #endif
#endif


extern pthread_mutex_t wmutex[1];
extern int sockfd;

static pthread_spinlock_t spin[1];
static fd_set rfds;
static fd_set wfds;
static fd_set exfds;
static int socklist[SELECT_MAX_CONNECTIONS];
static uchar sockmask[SELECT_MAX_CONNECTIONS];
static volatile uint socklist_len = 0;
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
	int locked = pthread_spin_trylock(spin);
	
	if (socklist_len >= SELECT_MAX_CONNECTIONS)
	{
		if (locked == 0)
			pthread_spin_unlock(spin);
		return;
	}
	socklist[socklist_len] = fd;
	sockmask[socklist_len] = SELECT_READ;
	socklist_len++;
	recalc_maxfd_plus_one();
	
	if (locked == 0)
		pthread_spin_unlock(spin);
}

inline void _select_socket_del_from_event_list (int fd)
{
	static uint i;
	
	int locked = pthread_spin_trylock(spin);
	
	for (i = 0; i < socklist_len; i++)
		if (socklist[i] == fd)
		{
			socklist_len--;
			socklist[i] = socklist[socklist_len];
			sockmask[i] = sockmask[socklist_len];
			break;
		}
	
	recalc_maxfd_plus_one();
	
	if (locked == 0)
		pthread_spin_unlock(spin);
}

static inline void set_select_event_mask (int fd, uchar mask)
{
	static uint i;
	
	int locked = pthread_spin_trylock(spin);
	
	for (i = 0; i < socklist_len; i++)
		if (socklist[i] == fd)
		{
			sockmask[i] = mask;
			break;
		}
	
	if (locked == 0)
		pthread_spin_unlock(spin);
}

void set_read_mask (int fd)
{
	set_select_event_mask(fd, SELECT_READ);
}

void set_write_mask (int fd)
{
	set_select_event_mask(fd, SELECT_WRITE);
}

void disable_events_for_socket (int fd)
{
	set_select_event_mask(fd, 0);
}

inline void end_request(request_t * r)
{
	#ifndef _WIN
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1 && errno != EBADF)
		perr("setsockopt(%d)", r->sock);
	#endif
	
	http_cleanup(r);
}

void event_routine (void)
{
	const int srvfd = sockfd;
	const int enable = 1;
	const long timeout_sec = EVENTS_WAIT_TIMEOUT / 1000L;
	const long timeout_usec = (EVENTS_WAIT_TIMEOUT % 1000L) * 1000L;
	int n, i, fd;
	socklen_t client_name_len;
	request_t * r;
	struct sockaddr * addr;
	struct linger linger_opt;
	struct timeval tv;
	
	pthread_spin_init(spin, PTHREAD_PROCESS_PRIVATE);
	
	linger_opt.l_onoff = 1;
	linger_opt.l_linger = 0;
	
	event_startup(&addr, &client_name_len);
	
	maxfd_plus_one = srvfd + 1;
	
	socklist_len = 0;
	
	for (;;)
	{
		event_iter();
		
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&exfds);
		FD_SET(srvfd, &rfds);
		
		pthread_spin_lock(spin);
		for (i = 0; i < socklist_len; i++)
		{
			#ifdef _WIN
			if (recv(socklist[i], NULL, 0, MSG_PEEK) == SOCKET_ERROR && socket_errno != WSAEWOULDBLOCK)
			#else
			if (fcntl(socklist[i], F_GETFL) == -1)
			#endif
			{
				#ifdef _WIN
				r = event_find_request(socklist[i]);
				
				if (r != NULL)
				{
					r->keepalive = false;
					end_request(r);
				}
				#else
				socklist_len--;
				socklist[i] = socklist[socklist_len];
				sockmask[i] = sockmask[socklist_len];
				recalc_maxfd_plus_one();
				#endif
				i--;
				continue;
			}
			FD_SET(socklist[i], &exfds);
			if (sockmask[i] == SELECT_READ)
				FD_SET(socklist[i], &rfds);
			else if (sockmask[i] == SELECT_WRITE)
				FD_SET(socklist[i], &wfds);
		}
		pthread_spin_unlock(spin);
		
		tv.tv_sec = timeout_sec;
		tv.tv_usec = timeout_usec;
		
		n = select(maxfd_plus_one, &rfds, &wfds, &exfds, &tv);
		
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
			#if 0
			}
			#endif
		}
		
		for (i = 0; i < socklist_len; i++)
		{
			pthread_spin_lock(spin);
			fd = socklist[i];
			pthread_spin_unlock(spin);
			
			if (FD_ISSET(fd, &exfds))
			{
				debug_print_3("select exeption on fd %d", fd);
				
				r = event_find_request(fd);
				
				if (r != NULL)
				{
					r->keepalive = false;
					remove_keepalive_socket(r->sock);
					end_request(r);
				}
			}
			else if (FD_ISSET(fd, &rfds))
			{
				r = event_fetch_request(fd);
				
				if (http_serve_client(r))
					end_request(r);
			}
			else if (FD_ISSET(socklist[i], &wfds))
			{
				events_out_data(fd);
			}
		}
	}
}

#endif
#endif
#endif
