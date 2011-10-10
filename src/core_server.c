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

#define _GNU_SOURCE
#define __USE_GNU
#ifndef _MSC_VER
 #include <sys/time.h>
#endif
#include <sys/resource.h> 
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "network_utils.h"
#include "common_functions.h"
#include "localization.h"
#ifdef HAVE_EPOLL
 #include <sys/epoll.h>
#endif
#include "sendfile.h"
#include "core_process.h"
#include "core_http.h"
#include "event_wrapper.h"
#include "mapped_memory.h"
#include "file_temporary_storage.h"
#include "web.h"
#include "win32_utils.h"


typedef struct request_vec
{
	request_t r;
	struct request_vec * next;
} request_vec_t;


bool ipv6_addr = false;
pid_t worker_pid = 0;
int sockfd;
static request_vec_t * requests_vector;
static uint maxfds;
static uint keepalive_max_conn;
static time_t server_start_time;
static frag_pool_t * limit_req_clients = NULL;
static frag_pool_t * limit_sim_req_clients = NULL;
static frag_pool_t * keepalive_sockets = NULL;
extern pthread_mutex_t wmutex[1];

static void quit_ex (int, int);

bool new_keepalive_socket (int sock)
{
	register uchar * bin1, * bin2;
	register uchar len;
	register uint i, total, percli;
	static socklen_t client_name_len;
	register keepalive_sock_t * k;
	register time_t curtime;
	static struct sockaddr addr, saddr;
	
	curtime = current_time_sec;
	
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
	register uint i;
	register keepalive_sock_t * k;
	
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

bool limit_requests (struct sockaddr * addr)
{
	register uchar * bin;
	register uchar len;
	register uint i;
	register limit_req_t * cli;
	register time_t curtime;
	
	curtime = current_time_sec;
	
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

bool limit_sim_requests (struct sockaddr * addr, socklen_t client_name_len, int sock)
{
	register uchar * bin1, * bin2;
	register uchar len;
	register uint i, total;
	register int * cli;
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

inline request_t * event_find_request (int sock)
{
	register request_vec_t * it;
	
	pthread_mutex_lock(wmutex);
	
	for (it = requests_vector; it != NULL; it = it->next)
		if (it->r.sock == sock)
		{
			pthread_mutex_unlock(wmutex);
			return &(it->r);
		}
	
	for (it = requests_vector; it != NULL; it = it->next)
		if (it->r.sock == -1)
		{
			it->r.sock = sock;
			pthread_mutex_unlock(wmutex);
			return &(it->r);
		}
	
	pthread_mutex_unlock(wmutex);
	
	return NULL;
}

request_t * event_fetch_request (int sock)
{
	register request_vec_t * it;
	register request_vec_t * new;
	
	pthread_mutex_lock(wmutex);
	
	for (it = requests_vector; it != NULL; it = it->next)
		if (it->r.sock == sock)
		{
			pthread_mutex_unlock(wmutex);
			return &(it->r);
		}
	
	for (it = requests_vector; it != NULL; it = it->next)
		if (it->r.sock == -1)
		{
			it->r.sock = sock;
			pthread_mutex_unlock(wmutex);
			return &(it->r);
		}
	
	for (it = requests_vector; it->next != NULL; it = it->next);
	
	new = (request_vec_t *) allocator_malloc(sizeof(request_vec_t));
	new->next = NULL;
	it->next = new;
	http_prepare(&(new->r), true);
	new->r.sock = sock;
	
	pthread_mutex_unlock(wmutex);
	
	debug_print_3("new request structure (%p)", new);
	
	return &(new->r);
}

void event_startup (struct sockaddr ** addr, socklen_t * client_name_len)
{
	uint i;
	request_vec_t * prealloc_request = NULL;
	request_vec_t * prev_prealloc_request = NULL;
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
		* client_name_len = sizeof(struct sockaddr_in6);
	else
	#endif
		* client_name_len = sizeof(struct sockaddr_in);
	
	* addr = (struct sockaddr *) allocator_malloc(* client_name_len);
	
	web_init();
	
	if (config.limit_req)
	{
		limit_req_clients = frag_pool_load(sizeof(limit_req_t), HTTP_LIMIT_REQUESTS_POOL_RESERVED_SIZE, ".http_limit_requests");
		if (limit_req_clients == NULL)
			limit_req_clients = frag_pool_create(sizeof(limit_req_t), HTTP_LIMIT_REQUESTS_POOL_RESERVED_SIZE);
	}
	if (config.limit_sim_req)
		limit_sim_req_clients = frag_pool_create(sizeof(int), HTTP_LIMIT_SIM_REQUESTS_POOL_RESERVED_SIZE);
	keepalive_sockets = frag_pool_create(sizeof(keepalive_sock_t), HTTP_KEEPALIVE_SOCKETS_POOL_RESERVED_SIZE);
	
	for (i = 0; i < config.prealloc_request_structures; i++)
	{
		prev_prealloc_request = prealloc_request;
		prealloc_request = (request_vec_t *) allocator_malloc(sizeof(request_vec_t));
		prealloc_request->next = NULL;
		
		if (prev_prealloc_request)
			prev_prealloc_request->next = prealloc_request;
		else
			requests_vector = prealloc_request;
		
		http_prepare(&(prealloc_request->r), false);
	}
}

inline void event_iter (void)
{
	register time_t curtime;
	register uint i;
	register int64 dlt;
	register keepalive_sock_t * k;
	register request_vec_t * it, * prev;
	
	curtime = current_time_sec;
	
	if (config.restart_timeout && curtime - server_start_time > config.restart_timeout)
	{
		#ifdef _WIN
		/* Special magic (hack) that makes the running service exit() with non-zero code. Then it automatically restart by service host. Maybe it's a dirty solution and should be rewritten. */
		extern bool win32_service_running;
		win32_service_running = false;
		#endif
		log_msg("Process lifetime limit (%ld) has reached. Restarting...", config.restart_timeout);
		quit_ex(0, WORKER_RESTART_STATUS_CODE);
	}
	
	for (i = 0; i < keepalive_sockets->real_len; i++)
	{
		if (keepalive_sockets->e[i].free)
			continue;
		
		k = (keepalive_sock_t *) keepalive_sockets->e[i].data;
		
		dlt = curtime - k->time;
		
		if (dlt > config.keepalive_timeout_val || dlt < 0)
		{
			for (it = requests_vector; it != NULL; it = it->next)
				if (it->r.sock == k->sock)
					break;
			if (it == NULL)
			{
				socket_close(k->sock);
				debug_print_3("keepalive-timeout expired, close(): %d", k->sock);
				frag_pool_free_alt(keepalive_sockets, i);
			}
		}
	}
	
	if (config.idle_request_structures)
	{
		for (it = requests_vector, prev = requests_vector, i = 0; i < config.prealloc_request_structures; i++, prev = it, it = it->next); /* skip preallocated request structures */
		
		for (; it != NULL; prev = it, it = it->next)
			if (it->r.sock == -1)
			{
				/* free idle structure, release memory */
				prev->next = it->next;
				http_destroy(&(it->r));
				allocator_free(it);
				it = prev;
			}
	}
	
	if (config.limit_req)
	{
		for (i = 0; i < limit_req_clients->real_len; i++)
			if (!(limit_req_clients->e[i].free) && ((limit_req_t *) limit_req_clients->e[i].data)->dtime != 0 && curtime > ((limit_req_t *) limit_req_clients->e[i].data)->dtime)
				frag_pool_free_alt(limit_req_clients, i);
	}
}

inline void end_request (request_t * r);

inline void events_out_data (int fd)
{
	register request_t * it;
	register uint t, d, size;
	register ssize_t res;
	
	for (it = (request_t *) requests_vector; it != NULL; it = (request_t *) ((request_vec_t *) it)->next)
		if (it->sock == fd)
		{
			if (it->temp.writev_total > 0)
			{
				res = writev(it->sock, it->temp.out_vec, it->temp.out_vec_len);
				if (res == -1)
				{
					perr("writev(): %d", -1);
					http_cleanup(it);
					break;
				}
				
				it->temp.writev_total -= res;
				
				for (t = 0, size = 0; t < it->temp.out_vec_len; t++)
				{
					size += it->temp.out_vec[t].iov_len;
					
					if (size >= res)
					{
						it->temp.out_vec += t;
						d = it->temp.out_vec[0].iov_len - (size - res);
						it->temp.out_vec[0].iov_base = ((uchar *) it->temp.out_vec[0].iov_base) + d;
						it->temp.out_vec[0].iov_len -= d;
						it->temp.out_vec_len = it->temp.out_vec_len - t;
						break;
					}
				}
				
				break;
			}
			
			#ifndef _WIN
			if (it->temp.sendfile_fd != -1)
			{
				#ifdef _BSD
				static off_t sbytes;
				sbytes = 0;
				res = sendfile(it->temp.sendfile_fd, it->sock, it->temp.sendfile_offset, it->temp.sendfile_last - it->temp.sendfile_offset, NULL, &sbytes, 0);
				it->temp.sendfile_offset += sbytes;
				#else
				res = sendfile(it->sock, it->temp.sendfile_fd, &(it->temp.sendfile_offset), it->temp.sendfile_last - it->temp.sendfile_offset);
				#endif
				
				if (unlikely(res == -1 && socket_wouldntblock(errno)))
				{
					perr("sendfile(): %d", (int) res);
					close(it->temp.sendfile_fd);
					http_cleanup(it);
					break;
				}
				
				if (it->temp.sendfile_offset < it->temp.sendfile_last)
					break;
				else
					close(it->temp.sendfile_fd);
			}
			#endif
			
			end_request(it);
			
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

void disable_events_for_socket (int sock)
{
	static struct epoll_event ev;
	
	ev.events = 0;
	ev.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);
}

inline void end_request (request_t * r)
{
	static const int disable = 0;
	
	if (* http_server_tcp_addr.str && setsockopt(r->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1 && errno != EBADF)
		perr("setsockopt(%d)", r->sock);
	
	http_cleanup(r);
}

static void event_routine (void)
{
	const int epollmaxevents = 100;
	const int srvfd = sockfd;
	const int enable = 1;
	int n, i;
	request_t * r;
	socklen_t client_name_len;
	struct epoll_event e[epollmaxevents], ev;
	struct sockaddr * addr;
	static struct linger linger_opt;
	
	memset(&ev, 0, sizeof(struct epoll_event));
	
	linger_opt.l_onoff = 1;
	linger_opt.l_linger = 0;
	
	event_startup(&addr, &client_name_len);
	
	epfd = epoll_create(epollmaxevents);
	
	if (epfd < 0)
		peerr(-1, "epoll_create(): %d", epfd);
	
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = srvfd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, srvfd, &ev) == -1)
		peerr(-1, "epoll_ctl(): %d", -1);
	
	for (;;)
	{
		event_iter();
		
		n = epoll_wait(epfd, e, epollmaxevents, EVENTS_WAIT_TIMEOUT);
		
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
				r = event_fetch_request(e[i].data.fd);
				
				if (http_serve_client(r))
					end_request(r);
			}
			else if (e[i].events == EPOLLOUT)
				events_out_data(e[i].data.fd);
			else if (e[i].events & EPOLLHUP)
			{
				debug_print_2("client %d reset connection", e[i].data.fd);
				
				r = event_find_request(e[i].data.fd);
				
				if (r != NULL)
				{
					pthread_mutex_lock(wmutex);
					r->keepalive = false;
					remove_keepalive_socket(r->sock);
					http_cleanup(r);
					pthread_mutex_unlock(wmutex);
				}
			}
		}
	}
}

#endif /* HAVE_EPOLL */

/* If we don't have epoll */
#include "event_wrapper_kqueue.h"
#include "event_wrapper_select.h"

static int connect_to_socket (void)
{
	int sockfd = 0, res = 0;
	const int enable = 1;
	uchar name_buf[128];
	struct sockaddr * name = (struct sockaddr *) name_buf;
	socklen_t name_len = net_gethostaddr(http_server_tcp_addr.str, http_port, AF_UNSPEC, name);
	
	sockfd = socket(name->sa_family, SOCK_STREAM, 0);
	
	if (sockfd < 0)
		peerr(-1, "socket(): %d", sockfd);
	
	#ifdef _WIN
	if (ioctlsocket(sockfd, FIONBIO, (void *) &enable) == -1)
		peerr(-1, "ioctlsocket(): %d", -1);
	#else
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
		peerr(-1, "fcntl(): %d", -1);
	#endif
	
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &enable, sizeof(enable))) < 0)
		peerr(-1, "setsockopt(): %d", res);
	
	#ifndef _WIN
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, &enable, sizeof(enable))) < 0)
		peerr(-1, "setsockopt(): %d", res);
	#endif
	
	if ((res = bind(sockfd, name, name_len)) < 0)
		peerr(-1, "bind(): %d", res);
	
	if ((res = listen(sockfd, LISTEN_BACKLOG)) < 0)
		peerr(-1, "listen(): %d", res);
	
	return sockfd;
}

volatile time_t current_time_sec;
volatile uint64 current_time_msec;

static void * _time_routine (void * ptr)
{
	const uchar upd_interval = 100;
	#ifdef _WIN
	SYSTEMTIME tm;
	#else
	struct timeval ctv;
	struct timespec tv;
	tv.tv_sec = 0;
	tv.tv_nsec = upd_interval * 1000000L;
	#endif
	
	ulong i = 0;
	
	for (;; i++)
	{
		#ifdef _WIN
		Sleep(upd_interval);
		#else
		(void) nanosleep(&tv, NULL);
		#endif
		
		if (unlikely(i % 50 == 0))
		{
			#ifdef _WIN
			GetSystemTime(&tm);
			current_time_sec = time(NULL);
			current_time_msec = (current_time_sec * 1000ULL) + tm.wMilliseconds;
			#else
			(void) gettimeofday(&ctv, NULL);
			current_time_sec = ctv.tv_sec;
			current_time_msec = (ctv.tv_sec * 1000ULL) + (ctv.tv_usec / 1000ULL);
			#endif
		}
		else
		{
			current_time_msec += 100;
			current_time_sec = current_time_msec / 1000;
		}
	}
	
	return NULL;
}

static void time_routine (void)
{
	#ifdef _WIN
	HANDLE hThread;
	
	current_time_sec = 0;
	current_time_msec = 0;
	
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) _time_routine, NULL, 0, NULL);
	SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
	#else
	const int policy = SCHED_FIFO;
	int r;
	pthread_t thread;
	pthread_attr_t attr[1];
	
	r = pthread_attr_init(attr);
	if (r != 0)
		eerr(-1, "pthread_attr_init(): %d", r);
	r = pthread_attr_setschedpolicy(attr, policy);
	assert(r == 0);
	struct sched_param param;
	memset(&param, 0, sizeof(param));
	param.sched_priority = sched_get_priority_min(policy);
	r = pthread_attr_setschedparam(attr, &param);
	assert(r == 0);
	
	current_time_sec = 0;
	current_time_msec = 0;
	
	r = pthread_create(&thread, attr, _time_routine, NULL);
	assert(r == 0);
	
	pthread_attr_destroy(attr);
	#endif
	
	while (current_time_sec == 0) {}
}

static void pr_set_limits (void)
{
	#ifdef HAVE_GETRLIMIT
	struct rlimit lim;
	
	if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
		peerr(-1, "getrlimit(): %d", -1);
	
	lim.rlim_cur = lim.rlim_max;
	
	if (setrlimit(RLIMIT_NOFILE, &lim) == -1)
		peerr(-1, "setrlimit(): %d", -1);
	
	maxfds = (uint) lim.rlim_cur;
	
	if (getrlimit(RLIMIT_CORE, &lim) == -1)
		peerr(-1, "getrlimit(): %d", -1);
	
	lim.rlim_cur = min(lim.rlim_max, CORE_DUMP_MAX_FILESIZE);
	
	if (setrlimit(RLIMIT_CORE, &lim) == -1)
		peerr(-1, "setrlimit(): %d", -1);
	
	debug_print_2("system limit: maximum core dump size: %u", (uint) lim.rlim_cur);
	#else
	maxfds = 1024;
	#endif
	
	debug_print_2("system limit: maximum file descriptors per process: %u", maxfds);
	
	keepalive_max_conn = max((maxfds - config.prealloc_request_structures) - 2, 2);
}

static void quit_ex (int prm, int exit_status)
{
	static bool already_in_quit = false;
	
	if (already_in_quit)
		return;
	
	already_in_quit = true;
	
	http_terminate();
	socket_close(sockfd);
	remove_pidfile();
	if (limit_req_clients != NULL)
		(void) frag_pool_save(limit_req_clients, ".http_limit_requests");
	#ifdef _WIN
	WSACleanup();
	if (worker_pid && worker_pid != (pid_t) getpid())
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, worker_pid);
		if (hProcess != NULL)
		{
			(void) TerminateProcess(hProcess, SIGTERM);
			CloseHandle(hProcess);
		}
	}
	#endif
	
	if (prm == SIGINT)
		fputc('\n', stdout);
	
	debug_print_1("terminate process: %d", prm);
	
	#ifdef _WIN
	_BEGIN_LOCAL_SECTION_
	extern bool win32_service_running;
	
	if (!win32_service_running)
		exit(exit_status);
	_END_LOCAL_SECTION_
	#else
	exit(exit_status);
	#endif
	
	already_in_quit = false;
}

void quit (int prm)
{
	quit_ex(prm, 0);
}

void init (char * procname)
{
	debug_print_3("%s...", "init");
	worker_pid = 0;
	
	mapped_memory_init();
	
	#if IPV6_SUPPORT
	if (* (http_server_tcp_addr.str) == '[' && * (http_server_tcp_addr.str + http_server_tcp_addr.len - 1) == ']')
	{
		char * garbage;
		
		garbage = http_server_tcp_addr.str;
		http_server_tcp_addr.str++;
		http_server_tcp_addr.str[http_server_tcp_addr.len - 2] = '\0';
		set_cpy_str(&http_server_tcp_addr, http_server_tcp_addr.str);
		allocator_free(garbage);
		ipv6_addr = true;
		
		debug_print_1("IPv6 enabled, addr \"%s\"", http_server_tcp_addr.str);
	}
	#endif
	
	#ifdef _WIN
	_BEGIN_LOCAL_SECTION_
	WSADATA wsaData;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		peerr(-1, "%s", "WSAStartup() failed");
	_END_LOCAL_SECTION_
	#endif
	
	sockfd = connect_to_socket();
	
	debug_print_2("connection established: %d", sockfd);
	
	#if DEBUG_LEVEL
	worker_pid = getpid();
	#else
	worker_pid = spawn_worker(procname);
	#endif
	
	setup_signals(quit);
	
	debug_print_2("worker process spawned successfully, PID is %d", worker_pid);
	
	server_start_time = time(NULL);
	
	pr_set_limits();
	time_routine();
	event_routine();
}
