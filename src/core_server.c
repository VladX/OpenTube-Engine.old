#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <resolv.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "common_functions.h"
#include "core_process.h"
#include "core_http.h"
#include "web.h"


bool ipv6_addr = false;
pid_t worker_pid = 0;
buf_t * uri_map;
static int sockfd;
static int epfd;

void set_epollout_event_mask (int sock)
{
	struct epoll_event ev;
	
	ev.events = EPOLLOUT;
	ev.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);
}

static void event_routine (void)
{
	const int maxevents = MAX_EVENTS;
	const int srvfd = sockfd;
	const int enable = 1;
	const int disable = 0;
	int n, i, it;
	uint t, d, size;
	ssize_t res;
	socklen_t client_name_len;
	request_t * request[maxevents];
	struct epoll_event e[maxevents], ev;
	struct sockaddr * addr;
	
	#if IPV6_SUPPORT
	if (ipv6_addr)
		client_name_len = sizeof(struct sockaddr_in6);
	else
	#endif
		client_name_len = sizeof(struct sockaddr_in);
	
	addr = (struct sockaddr *) malloc(client_name_len);
	
	uri_map = buf_create(sizeof(uri_map_t), 10);
	
	web_init();
	
	for (i = 0; i < maxevents; i++)
	{
		request[i] = (request_t *) malloc(sizeof(request_t));
		
		http_prepare(request[i]);
	}
	
	epfd = epoll_create(maxevents);
	
	if (epfd < 0)
		peerr(17, "epoll_create(): %d", epfd);
	
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = srvfd;
	
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, srvfd, &ev) == -1)
		peerr(18, "epoll_ctl(): %d", -1);
	
	for (;;)
	{
		n = epoll_wait(epfd, e, maxevents, -1);
		
		for (i = 0; i < n; i++)
		{
			if (e[i].data.fd == srvfd)
			{
				#if HAVE_ACCEPT4
				while ((ev.data.fd = accept4(srvfd, addr, &client_name_len, SOCK_NONBLOCK)) != -1)
				{
				#else
				while ((ev.data.fd = accept(srvfd, addr, &client_name_len)) != -1)
				{
					fcntl(ev.data.fd, F_SETFL, O_NONBLOCK);
				#endif
					debug_print_3("accept(): %d", ev.data.fd);
					ev.events = EPOLLIN;
					epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
					
					if (* http_server_tcp_addr.str && setsockopt(ev.data.fd, IPPROTO_TCP, TCP_CORK, &enable, sizeof(enable)) == -1)
						perr("setsockopt(): %d", -1);
				}
				continue;
			}
			else if (e[i].events == EPOLLIN)
			{
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == e[i].data.fd)
						goto _h_req;
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == -1)
						goto _h_req;
				continue;
				_h_req:;
				
				request[it]->sock = e[i].data.fd;
				
				if (request[it]->temp.tempfd != -1)
				{
					if (http_temp_file(request[it]))
					{
						if (* http_server_tcp_addr.str && setsockopt(request[it]->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1)
							perr("setsockopt(): %d", -1);
						close(request[it]->sock);
						http_cleanup(request[it]);
					}
					continue;
				}
				
				if (http_serve_client(request[it]))
				{
					if (* http_server_tcp_addr.str && setsockopt(request[it]->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1)
						perr("setsockopt(): %d", -1);
					close(request[it]->sock);
					http_cleanup(request[it]);
				}
			}
			else
			{
				for (it = 0; it < maxevents; it++)
					if (request[it]->sock == e[i].data.fd)
					{
						if (request[it]->temp.writev_total > 0)
						{
							res = writev(request[it]->sock, request[it]->temp.out_vec, request[it]->temp.out_vec_len);
							if (res == -1)
							{
								perr("writev(): %d", -1);
								close(request[it]->sock);
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
							res = sendfile(request[it]->sock, request[it]->temp.sendfile_fd, &(request[it]->temp.sendfile_offset), request[it]->temp.sendfile_last - request[it]->temp.sendfile_offset);
						
							if (res == -1)
							{
								perr("sendfile(): %d", (int) res);
								close(request[it]->sock);
								http_cleanup(request[it]);
								break;
							}
							
							if (request[it]->temp.sendfile_offset < request[it]->temp.sendfile_last)
								break;
						}
						
						if (* http_server_tcp_addr.str && setsockopt(request[it]->sock, IPPROTO_TCP, TCP_CORK, &disable, sizeof(disable)) == -1)
							perr("setsockopt(): %d", -1);
						close(request[it]->sock);
						http_cleanup(request[it]);
						
						break;
					}
			}
		}
	}
}

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
		struct sockaddr_un * uname;
		
		sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sockfd < 0)
			peerr(7, "socket(): %d", sockfd);
		uname = (struct sockaddr_un *) malloc(sizeof(* uname));
		memset(uname, 0, sizeof(* uname));
		uname->sun_family = AF_UNIX;
		memcpy(uname->sun_path, http_server_unix_addr.str, http_server_unix_addr.len + 1);
		name = (struct sockaddr *) uname;
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
	
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
		peerr(7, "fcntl(): %d", -1);
	
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) < 0)
		peerr(7, "setsockopt(): %d", res);
	
	if ((res = setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, &enable, sizeof(enable))) < 0)
		peerr(7, "setsockopt(): %d", res);
	
	if ((res = bind(sockfd, name, name_len)) < 0)
		peerr(7, "bind(): %d", res);
	
	if ((res = listen(sockfd, LISTEN_BACKLOG)) < 0)
		peerr(7, "listen(): %d", res);
	
	return sockfd;
}

void quit (int prm)
{
	close(sockfd);
	fputc('\n', stdout);
	debug_print_1("terminate process: %d", prm);
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
	
	sockfd = connect_to_socket();
	
	debug_print_2("connection established: %d", sockfd);
	
	if (signal(SIGINT, quit) == SIG_ERR)
		err("can't handle signal %d", SIGINT);
	if (signal(SIGTERM, quit) == SIG_ERR)
		err("can't handle signal %d", SIGTERM);
	if (signal(SIGQUIT, quit) == SIG_ERR)
		err("can't handle signal %d", SIGQUIT);
	
	worker_pid = spawn_worker(procname);
	
	debug_print_2("worker process spawned successfully, PID is %d", worker_pid);
	
	event_routine();
}
