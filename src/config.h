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

#define PROG_NAME "Opentube"
#define PROG_VERSION "0.1alpha"
#define DEBUG_LEVEL 3 // 0 - disabled
#define SERVER_STRING PROG_NAME "/" PROG_VERSION
#define IPV6_SUPPORT 1
#define WORKER_THREADS 10
#define MAX_EVENTS 10
#define CONFIG_PATH "./server.conf"
#define LISTEN_BACKLOG 500
#define HTTP_BUFFER_RESERVED_SIZE 262144
#define HTTP_RECV_BUFFER 8192
#define HTTP_HEADERS_POOL_RESERVED_FRAGMENTS 32
#define HTTP_GZIP_BUFFER_RESERVED_SIZE 65536
#define HTTP_CLIENTS_POOL_RESERVED_SIZE 128
#define HTTP_BODY_SIZE_WRITE_TO_FILE 8388608 // 8 Mb
#define HTTP_MAX_REQUEST_BODY_SIZE 33554432 // 32 Mb
#define HTTP_MAX_HEADERS_SIZE 16384
#define HTTP_STATIC_EXPIRES 172800 // 2 days
#define HTTP_PATH_PREALLOC 128
#define EPOLL_TIMEOUT 2500

#define HAVE_ACCEPT4 1
