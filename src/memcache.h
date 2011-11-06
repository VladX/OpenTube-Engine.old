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

#ifndef _INT_MEMCACHE_H
#define _INT_MEMCACHE_H 1

typedef u_str_t * (* cache_update_cb) (time_t, u_str_t *);

u_str_t * cache_find (u_str_t * name, time_t * time, bool * accept_gzip);

void cache_store (u_str_t * name, u_str_t * data, cache_update_cb update_callback);

void cache_update (u_str_t * name);

void cache_free (void);

void cache_create (void);

#endif
