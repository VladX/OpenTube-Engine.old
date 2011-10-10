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

#ifndef EVENT_WRAPPER_H
#define EVENT_WRAPPER_H

extern inline request_t * event_find_request (int sock);

extern inline request_t * event_fetch_request (int sock);

void event_startup (struct sockaddr ** addr, socklen_t * client_name_len);

extern inline void event_iter (void);

extern inline bool limit_requests (struct sockaddr * addr);

extern inline bool limit_sim_requests (struct sockaddr * addr, socklen_t client_name_len, int sock);

extern inline void events_out_data (int fd);

#endif
