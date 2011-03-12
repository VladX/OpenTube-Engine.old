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

#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h> 
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <resolv.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "common_functions.h"
#ifdef HAVE_EPOLL
 #include <sys/epoll.h>
#endif
#include "sendfile.h"
#include "core_process.h"
#include "core_http.h"
#include "web.h"
#include "win32_utils.h"


bool ipv6_addr = false;
pid_t worker_pid = 0;
int sockfd;
static uint maxfds;
static uint keepalive_max_conn;
static frag_pool_t * limit_req_clients;
static frag_pool_t * limit_sim_req_clients;
static frag_pool_t * keepalive_sockets;
extern pthread_mutex_t wmutex[1];


bool new_keepalive_socket (int sock)
{
	static uchar * bin1, * bin2;
	static uchar len;
	static uint i, total, percli;
	static socklen_t client_name_len;
	static keepalive_sock_t * k;
	static time_t curtime;
	static struct sockaddr addr, saddr;
	
	curtime = time(NULL);
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
	{
		bin1 = (uchar *) ((struct sockaddr_in6 *) &addr)->sin6_addr.s6_addr;
		bin2 = (uchar *) ((struct sockaddr_in6 *) &saddr)->sin6_addr.s6_addr;
		client_name_len = sizeof(struct sockaddr_in6);
		len = 16;
	}
	else
	#endif
	{
		bin1 = (uchar *) &(((struct sockaddr_in *) &addr)->sin_addr.s_addr);
		bin2 = (uchar *) &(((struct sockaddr_in *) &saddr)->sin_addr.s_addr);
		client_name_len = sizeof(struct sockaddr_in);
		len = 4;
	}
	
	if (getpeername(sock, &addr, &client_name_len) == -1)
		return false;
	
	for (i = 0, total = 0, percli = 0; i < keepalive_sockets->real_len; i++)
	{
		if (keepalive_sockets->e[i].free)
			continue;
		
		k = (keepalive_sock_t *) keepalive_sockets->e[i].data;
		
		if (k->sock == sock)
		{
			k->time = curtime;
			
			return true;
		}
		
		if (getpeername(k->sock, &saddr, &client_name_len) == -1)
			continue;
		
		if (memcmp(bin1, bin2, len) == 0)
			percli++;
		
		total++;
	}
	
	if (percli >= config.keepalive_max_conn_per_client)
		return false;
	
	if (total >= keepalive_max_conn)
		return false;
	
	k = (keepalive_sock_t *) frag_pool_alloc(keepalive_sockets);
	k->sock = sock;
	k->time = curtime;
	
	return true;
}

void remove_keepalive_socket(int sock)
{
	static uint i;
	static keepalive_sock_t * k;
	
	for (i = 0; i < keepalive_sockets->real_len; i++)
	{
		if (keepalive_sockets->e[i].free)
			continue;
		
		k = (keepalive_sock_t *) keepalive_sockets->e[i].data;
		
		if (k->sock == sock)
		{
			frag_pool_free_alt(keepalive_sockets, i);
			
			return;
		}
	}
}

inline bool limit_requests (struct sockaddr * addr)
{
	static uchar * bin;
	static uchar len;
	static uint i;
	static limit_req_t * cli;
	static time_t curtime;
	
	curtime = time(NULL);
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
	{
		bin = (uchar *) ((struct sockaddr_in6 *) addr)->sin6_addr.s6_addr;
		len = 16;
	}
	else
	#endif
	{
		bin = (uchar *) &(((struct sockaddr_in *) addr)->sin_addr.s_addr);
		len = 4;
	}
	
	for (i = 0; i < limit_req_clients->real_len; i++)
	{
		if (limit_req_clients->e[i].free)
			continue;
		
		cli = (limit_req_t *) limit_req_clients->e[i].data;
		
		if (memcmp(cli->addr, bin, len) == 0)
		{
			if (cli->req > config.limit_rate)
				return true;
			
			if (curtime - cli->time <= 1)
			{
				cli->req++;
				
				if (cli->req > config.limit_rate)
				{
					cli->dtime = curtime + config.limit_delay;
					
					return true;
				}
				
				return false;
			}
			
			frag_pool_free_alt(limit_req_clients, i);
			
			return false;
		}
	}
	
	cli = (limit_req_t *) frag_pool_alloc(limit_req_clients);
	memcpy(cli->addr, bin, len);
	cli->time = curtime;
	cli->req = 1;
	cli->dtime = 0;
	
	return false;
}

inline bool limit_sim_requests (struct sockaddr * addr, socklen_t client_name_len, int sock)
{
	static uchar * bin1, * bin2;
	static uchar len;
	static uint i, total;
	static int * cli;
	static struct sockaddr saddr;
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
	{
		bin1 = (uchar *) ((struct sockaddr_in6 *) addr)->sin6_addr.s6_addr;
		bin2 = (uchar *) ((struct sockaddr_in6 *) &saddr)->sin6_addr.s6_addr;
		len = 16;
	}
	else
	#endif
	{
		bin1 = (uchar *) &(((struct sockaddr_in *) addr)->sin_addr.s_addr);
		bin2 = (uchar *) &(((struct sockaddr_in *) &saddr)->sin_addr.s_addr);
		len = 4;
	}
	
	for (i = 0, total = 0; i < limit_sim_req_clients->real_len; i++)
	{
		if (limit_sim_req_clients->e[i].free)
			continue;
		
		cli = (int *) limit_sim_req_clients->e[i].data;
		
		if (* cli == sock || getpeername(* cli, &saddr, &client_name_len) == -1)
		{
			frag_pool_free_alt(limit_sim_req_clients, i);
			
			continue;
		}
		
		if (memcmp(bin1, bin2, len) == 0)
		{
			total++;
			if (total >= config.limit_sim_threshold)
				return true;
		}
	}
	
	cli = (int *) frag_pool_alloc(limit_sim_req_clients);
	* cli = sock;
	
	return false;
}

void event_startup (const int maxevents, request_t ** request, struct sockaddr ** addr, socklen_t * client_name_len)
{
	uint i;
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
		* client_name_len = sizeof(struct sockaddr_in6);
	else
	#endif
		* client_name_len = sizeof(struct sockaddr_in);
	
	* addr = (struct sockaddr *) malloc(* client_name_len);
	
	web_init();
	
	if (config.limit_req)
		limit_req_clients = frag_pool_create(sizeof(limit_req_t), HTTP_LIMIT_REQUESTS_POOL_RESERVED_SIZE);
	if (config.limit_sim_req)
		limit_sim_req_clients = frag_pool_create(sizeof(int), HTTP_LIMIT_SIM_REQUESTS_POOL_RESERVED_SIZE);
	keepalive_sockets = frag_pool_create(sizeof(keepalive_sock_t), HTTP_KEEPALIVE_SOCKETS_POOL_RESERVED_SIZE);
	
	for (i = 0; i < maxevents; i++)
	{
		request[i] = (request_t *) malloc(sizeof(request_t));
		
		http_prepare(request[i]);
	}
}

inline void event_iter (request_t ** request)
{
	static time_t curtime;
	static uint i, it;
	static int dlt;
	static keepalive_sock_t * k;
	
	curtime = time(NULL);
	
	for (i = 0; i < keepalive_sockets->real_len; i++)
	{
		if (keepalive_sockets->e[i].free)
			continue;
		
		k = (keepalive_sock_t *) keepalive_sockets->e[i].data;
		
		dlt = curtime - k->time;
		
		if (dlt > config.keepalive_timeout_val || dlt < 0)
		{
			for (it = 0; it < MAX_EVENTS; it++)
				if (request[it]->sock == k->sock)
					break;
			if (it == MAX_EVENTS)
			{
				socket_close(k->sock);
				debug_print_3("keepalive-timeout expired, close(): %d", k->sock);
				frag_pool_free_alt(keepalive_sockets, i);
			}
		}
	}
	
	if (config.limit_req)
	{
		for (i = 0; i < limit_req_clients->real_len; i++)
			if (!(limit_req_clients->e[i].free) && ((limit_req_t *) limit_req_clients->e[i].data)->dtime != 0 && curtime > ((limit_req_t *) limit_req_clients->e[i].data)->dtime)
				frag_pool_free_alt(limit_req_clients, i);
	}
}

inline void end_request(request_t * r);

inline void events_out_data (const int maxevents, int fd, request_t ** request)
{
	static uint it;
	static uint t, d, size;
	static ssize_t res;
	
	for (it = 0; it < maxevents; it++)
		if (request[it]->sock == fd)
		{
			if (request[it]->temp.writev_total > 0)
			{
				res = writev(request[it]->sock, request[it]->temp.out_vec, request[it]->temp.out_vec_len);
				if (res == -1)
				{
					perr("writev(): %d", -1);
					http_cleanup(request[it]);
					break;
				}
				
				request[it]->temp.writev_total -= res;
				
				for (t = 0, size = 0; t < request[it]->temp.out_vec_len; t++)
				{
					size += request[it]->temp.out_vec[t].iov_len;
					
					if (size >= res)
					{
						request[it]->temp.out_vec += t;
						d = request[it]->temp.out_vec[0].iov_len - (size - res);
						request[it]->temp.out_vec[0].iov_base = ((uchar *) request[it]->temp.out_vec[0].iov_base) + d;
						request[it]->temp.out_vec[0].iov_len -= d;
						request[it]->temp.out_vec_len = request[it]->temp.out_vec_len - t;
						break;
					}
				}
				
				break;
			}
			
			if (request[it]->temp.sendfile_fd != -1)
			{
				#ifdef _BSD
				static off_t sbytes = 0;
				res = sendfile(request[it]->temp.sendfile_fd, request[it]->sock, request[it]->temp.sendfile_offset, request[it]->temp.sendfile_last - request[it]->temp.sendfile_offset, NULL, &sbytes, 0);
				request[it]->temp.sendfile_offset += sbytes;
				#else
				res = sendfile(request[it]->sock, request[it]->temp.sendfile_fd, &(request[it]->temp.sendfile_offset), request[it]->temp.sendfile_last - request[it]->temp.sendfile_offset);
				#endif
				
				if (res == -1)
				{
					perr("sendfile(): %d", (int) res);
					close(request[it]->temp.sendfile_fd);
					http_cleanup(request[it]);
					break;
				}
				
				if (request[it]->temp.sendfile_offset < request[it]->temp.sendfile_last)
					break;
				else
					close(request[it]->temp.sendfile_fd);
			}
			
			end_request(request[it]);
			
			break;
		}
}

#ifdef HAVE_EPOLL

static int epfd;

void set_epollout_event_mask (int sock)
{
	static struct epoll_event ev;
	
	ev.events = EPOLLOUT;
	ev.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);
}

void set_epollin_event_mask (int sock)
{
	static struct epoll_event ev;
	
	ev.events = EPOLLIN;
	ev.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);
}

inline void end_request(request_t * r)
{
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1)
		perr("setsockopt(%d)", r->sock);
	
	http_cleanup(r);
}

static void event_routine (void)
{
	const int maxevents = MAX_EVENTS;
	const int srvfd = sockfd;
	const int enable = 1;
	int n, i, it;
	socklen_t client_name_len;
	request_t * request[maxevents];
	struct epoll_event e[maxevents], ev;
	struct sockaddr * addr;
	struct linger linger_opt;
	
	linger_opt.l_onoff = 1;
	linger_opt.l_linger = 0;
	
	event_startup(maxevents, request, &addr, &client_name_len);
	
	epfd = epoll_create(maxevents);
	
	if (epfd < 0)
		peerr(17, "epoll_create(): %d", epfd);
	
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = srvfd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, srvfd, &ev) == -1)
		peerr(18, "epoll_ctl(): %d", -1);
	
	for (;;)
	{
		event_iter(request);
		
		n = epoll_wait(epfd, e, maxevents, EPOLL_TIMEOUT);
		
		for (i = 0; i < n; i++)
		{
			if (e[i].data.fd == srvfd)
			{
				#ifdef HAVE_ACCEPT4
				while ((ev.data.fd = accept4(srvfd, addr, &client_name_len, SOCK_NONBLOCK)) != -1)
				{
				#else
				while ((ev.data.fd = accept(srvfd, addr, &client_name_len)) != -1)
				{
					(void) fcntl(ev.data.fd, F_SETFL, O_NONBLOCK);
				#endif
					debug_print_2("accept(): %d", ev.data.fd);
					if (config.limit_req && limit_requests(addr))
					{
						debug_print_2("client has exceeded the allowable requests per second (rps) limit, request %d discarded", ev.data.fd);
						setsockopt(ev.data.fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
						socket_close(ev.data.fd);
						break;
					}
					if (config.limit_sim_req && limit_sim_requests(addr, client_name_len, ev.data.fd))
					{
						debug_print_2("client has exceeded the allowable simultaneous requests limit, request %d discarded", ev.data.fd);
						setsockopt(ev.data.fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
						socket_close(ev.data.fd);
						break;
					}
					ev.events = EPOLLIN;
					epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
					
					if (* http_server_tcp_addr.str && setsockopt(ev.data.fd, IPPROTO_TCP, TCP_CORK, &enable, sizeof(enable)) == -1)
						perr("setsockopt(): %d", -1);
				}
				continue;
			}
			else if (e[i].events == EPOLLIN)
			{
				pthread_mutex_lock(wmutex);
				
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == e[i].data.fd)
						goto _h_req;
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == -1)
						goto _h_req;
				pthread_mutex_unlock(wmutex);
				continue;
				_h_req:
				
				pthread_mutex_unlock(wmutex);
				
				request[it]->sock = e[i].data.fd;
				
				if (http_serve_client(request[it]))
					end_request(request[it]);
			}
			else
			{
				pthread_mutex_lock(wmutex);
				events_out_data(maxevents, e[i].data.fd, request);
				pthread_mutex_unlock(wmutex);
			}
		}
	}
}

#else
 #ifdef HAVE_SELECT
  #include "event_wrapper_select.h"
 #endif
#endif

static bool gethostaddr (char * name, const int type, in_addr_t * dst)
{
	int len;
	struct hostent * host;
	
	#if IPV6_SUPPORT
	 #ifdef RES_USE_INET6
	if (type == AF_INET6)
	{
		res_init();
		_res.options |= RES_USE_INET6;
	}
	 #endif
	#endif
	host = gethostbyname(name);
	if (!host || type != host->h_addrtype)
		return false;
	if (type == AF_INET6)
		len = 16;
	else
		len = sizeof(* dst);
	memcpy(dst, host->h_addr_list[0], len);
	
	#if DEBUG_LEVEL > 2
	struct in_addr addr;
	#if IPV6_SUPPORT
	struct in6_addr addr6;
	#endif
	char * addr_str = NULL;
	
	#if IPV6_SUPPORT
	if (type == AF_INET6)
	{
		memcpy(&(addr6.s6_addr), dst, len);
		addr_str = (char *) malloc(INET6_ADDRSTRLEN);
		addr_str = (char *) inet_ntop(AF_INET6, &addr6, addr_str, INET6_ADDRSTRLEN);
	}
	else
	{
	#endif
		addr.s_addr = * dst;
		addr_str = inet_ntoa(addr);
	#if IPV6_SUPPORT
	}
	#endif
	debug_print_3("gethostaddr(): %s -> %s", name, addr_str);
	#endif
	
	return true;
}

static int connect_to_socket (void)
{
	int sockfd, res;
	const int enable = 1;
	struct sockaddr * name;
	socklen_t name_len = sizeof(* name);
	
	if (* http_server_unix_addr.str)
	{
		#ifdef _WIN
		eerr(1, "%s", "Unix domains are not supported on Windows.");
		#else
		struct sockaddr_un * uname;
		
		sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sockfd < 0)
			peerr(7, "socket(): %d", sockfd);
		uname = (struct sockaddr_un *) malloc(sizeof(* uname));
		memset(uname, 0, sizeof(* uname));
		uname->sun_family = AF_UNIX;
		memcpy(uname->sun_path, http_server_unix_addr.str, http_server_unix_addr.len + 1);
		name = (struct sockaddr *) uname;
		#endif
	}
	#if IPV6_SUPPORT
	else if (* http_server_tcp_addr.str && ipv6_addr)
	{
		struct sockaddr_in6 * i6name;
		struct in6_addr addr;
		
		sockfd = socket(AF_INET6, SOCK_STREAM, 0);
		if (sockfd < 0)
			peerr(7, "socket(): %d", sockfd);
		i6name = (struct sockaddr_in6 *) malloc(sizeof(* i6name));
		memset(i6name, 0, sizeof(* i6name));
		i6name->sin6_family = AF_INET6;
		i6name->sin6_port = htons(http_port);
		if (!gethostaddr(http_server_tcp_addr.str, AF_INET6, (in_addr_t *) &(i6name->sin6_addr.s6_addr)))
		{
			if (inet_pton(AF_INET6, http_server_tcp_addr.str, &addr) <= 0)
				peerr(8, "Invalid address: %s", http_server_tcp_addr.str);
			memcpy(&(i6name->sin6_addr), &addr, sizeof(addr));
		}
		name = (struct sockaddr *) i6name;
		name_len = sizeof(* i6name);
	}
	#endif
	else
	{
		struct sockaddr_in * iname;
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			peerr(7, "socket(): %d", sockfd);
		iname = (struct sockaddr_in *) malloc(sizeof(* iname));
		memset(iname, 0, sizeof(* iname));
		iname->sin_family = AF_INET;
		iname->sin_port = htons(http_port);
		if (!gethostaddr(http_server_tcp_addr.str, AF_INET, &(iname->sin_addr.s_addr)))
		{
			#ifdef __USE_MISC
			if (inet_aton(http_server_tcp_addr.str, (struct in_addr *) &(iname->sin_addr.s_addr)) == 0)
			#else
			iname->sin_addr.s_addr = inet_addr(http_server_tcp_addr.str);
			if (iname->sin_addr.s_addr == INADDR_NONE && strcmp(http_server_tcp_addr.str, "255.255.255.255") != 0)
			#endif
				eerr(8, "Invalid address: %s", http_server_tcp_addr.str);
		}
		name = (struct sockaddr *) iname;
	}
	
	#ifdef _WIN
	if (ioctlsocket(sockfd, FIONBIO, &enable) == -1)
		peerr(7, "ioctlsocket(): %d", -1);
	#else
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
		peerr(7, "fcntl(): %d", -1);
	#endif
	
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) < 0)
		peerr(7, "setsockopt(): %d", res);
	
	#ifndef _WIN
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, &enable, sizeof(enable))) < 0)
		peerr(7, "setsockopt(): %d", res);
	#endif
	
	if ((res = bind(sockfd, name, name_len)) < 0)
		peerr(7, "bind(): %d", res);
	
	if ((res = listen(sockfd, LISTEN_BACKLOG)) < 0)
		peerr(7, "listen(): %d", res);
	
	return sockfd;
}

#ifdef _WIN
static void win32_exit_function (void)
{
	WSACleanup();
}
#endif

void quit (int prm)
{
	socket_close(sockfd);
	fputc('\n', stdout);
	debug_print_1("terminate process: %d", prm);
	#ifdef _WIN
	win32_exit_function();
	#endif
	exit(prm);
}

void init (char * procname)
{
	debug_print_3("%s...", "init");
	
	#if IPV6_SUPPORT
	if (* (http_server_tcp_addr.str) == '[' && * (http_server_tcp_addr.str + http_server_tcp_addr.len - 1) == ']')
	{
		char * garbage;
		
		garbage = http_server_tcp_addr.str;
		http_server_tcp_addr.str++;
		http_server_tcp_addr.str[http_server_tcp_addr.len - 2] = '\0';
		set_cpy_str(&http_server_tcp_addr, http_server_tcp_addr.str);
		free(garbage);
		ipv6_addr = true;
		
		debug_print_1("IPv6 enabled, addr \"%s\"", http_server_tcp_addr.str);
	}
	#endif
	
	#ifdef _WIN
	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		peerr(1, "%s", "WSAStartup() failed");
	#endif
	
	sockfd = connect_to_socket();
	
	debug_print_2("connection established: %d", sockfd);
	
	if (signal(SIGINT, quit) == SIG_ERR)
		err("can't handle signal %d", SIGINT);
	if (signal(SIGTERM, quit) == SIG_ERR)
		err("can't handle signal %d", SIGTERM);
	#ifndef _WIN
	if (signal(SIGQUIT, quit) == SIG_ERR)
		err("can't handle signal %d", SIGQUIT);
	#else
	atexit(win32_exit_function);
	#endif
	
	#if DEBUG_LEVEL
	worker_pid = getpid();
	#else
	worker_pid = spawn_worker(procname);
	#endif
	
	debug_print_2("worker process spawned successfully, PID is %d", worker_pid);
	
	#ifdef _WIN
	maxfds = 1024;
	#else
	
	struct rlimit lim;
	
	if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
		peerr(0, "getrlimit(): %d", -1);
	
	lim.rlim_cur = lim.rlim_max;
	
	if (setrlimit(RLIMIT_NOFILE, &lim) == -1)
		peerr(0, "setrlimit(): %d", -1);
	
	maxfds = (uint) lim.rlim_cur;
	#endif
	
	debug_print_2("system limit: maximum file descriptors per process: %u", maxfds);
	
	keepalive_max_conn = max((maxfds - MAX_EVENTS) - 2, 2);
	
	event_routine();
}
