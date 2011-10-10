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

#ifndef CORE_SERVER_H
#define CORE_SERVER_H 1

#include "event_wrapper_kqueue.h"
#include "event_wrapper_select.h"

#ifdef HAVE_EPOLL

#define socket_del_from_event_list(SOCKET)

void set_epollout_event_mask (int sock);

void set_epollin_event_mask (int sock);

void disable_events_for_socket (int sock);

void end_request(request_t * r);
#endif
bool new_keepalive_socket (int sock);

void remove_keepalive_socket(int sock);

void init (char * procname);

void quit (int prm);

#endif
