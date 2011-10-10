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

#include "common_functions.h"

#ifndef HAVE_EPOLL
#ifndef HAVE_KQUEUE
#ifdef HAVE_SELECT

#define set_epollout_event_mask set_write_mask
#define set_epollin_event_mask set_read_mask
#define socket_del_from_event_list(SOCKET) _select_socket_del_from_event_list(SOCKET)

extern inline void _select_socket_del_from_event_list (int fd);

void set_read_mask (int fd);

void set_write_mask (int fd);

void disable_events_for_socket (int fd);

void end_request(request_t * r);

void event_routine (void);

#endif
#endif
#endif
