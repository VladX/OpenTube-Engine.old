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
#include <time.h>
#include "endianness.h"
#include "win32_utils.h"
#include "libs/zlib.h"

#define GZIP_BUFFER_SIZE 512


typedef u_str_t * (* cache_update_cb) (time_t, u_str_t *);

typedef struct
{
	u_str_t key;
	u_str_t value;
	u_str_t gzipped_value;
	cache_update_cb update_callback;
	time_t time;
} cache_t;

static buf_t * cache = NULL;
static buf_t * cache_names = NULL;
static z_stream gz_strm[1];

static void gen_gzipped_value (cache_t * c)
{
	static const char gzip_header[10] = GZIP_HEADER;
	static uint32_t gzip_ending[2];
	
	uint len = 0;
	int r;
	
	gz_strm->avail_in = c->value.len;
	gz_strm->next_in = c->value.str;
	
	do
	{
		len += GZIP_BUFFER_SIZE;
		gz_strm->next_in = (c->value.str = allocator_realloc(gz_strm->next_in, gz_strm->avail_in + len + 18));
		gz_strm->avail_out = GZIP_BUFFER_SIZE;
		gz_strm->next_out = gz_strm->next_in + ((gz_strm->avail_in + len + 10) - GZIP_BUFFER_SIZE);
		r = deflate(gz_strm, Z_FINISH);
		if (r == Z_STREAM_END)
			break;
		if (r != Z_OK)
		{
			err("deflate(): %d", r);
			c->value.str = allocator_realloc(c->value.str, c->value.len);
			c->gzipped_value.str = NULL;
			c->gzipped_value.len = 0;
			return;
		}
	}
	while (gz_strm->avail_out == 0);
	
	if (gz_strm->avail_out)
	{
		len -= gz_strm->avail_out;
		c->value.str = allocator_realloc(c->value.str, c->value.len + len + 18);
	}
	
	c->gzipped_value.str = c->value.str + c->value.len;
	c->gzipped_value.len = len + 18;
	
	memcpy(c->gzipped_value.str, gzip_header, 10);
	
	gzip_ending[0] = crc32(0, c->value.str, c->value.len);
	gzip_ending[1] = c->value.len;
	#if __BYTE_ORDER == __BIG_ENDIAN
	gzip_ending[0] = bswap_32(gzip_ending[0]);
	gzip_ending[1] = bswap_32(gzip_ending[1]);
	#endif
	memcpy(c->gzipped_value.str + len + 10, gzip_ending, 8);
	
	(void) deflateReset(gz_strm);
}

u_str_t * cache_find (u_str_t * name, bool accept_gzip)
{
	register uint i;
	register cache_t * c;
	
	for (i = 0; i < cache->cur_len; i++)
	{
		c = &(((cache_t *) cache->data)[i]);
		if (name->len == c->key.len && memcmp(name->str, c->key.str, name->len) == 0)
		{
			debug_print_2("\"%s\" loaded from cache.", (char *) c->key.str);
			if (accept_gzip && c->gzipped_value.len)
				return &(c->gzipped_value);
			else
				return &(c->value);
		}
	}
	
	return NULL;
}

void cache_update (u_str_t * name)
{
	register uint i;
	register cache_t * c;
	
	for (i = 0; i < cache->cur_len; i++)
	{
		c = &(((cache_t *) cache->data)[i]);
		if (name->len == c->key.len && memcmp(name->str, c->key.str, name->len) == 0)
		{
			u_str_t * res;
			
			if (c->update_callback == NULL)
				return;
			
			res = c->update_callback(c->time, name);
			
			if (res != NULL)
			{
				c->value.str = allocator_realloc(c->value.str, res->len);
				memcpy(c->value.str, res->str, res->len);
				c->value.len = res->len;
				c->time = current_time_sec;
				gen_gzipped_value(c);
			}
			
			return;
		}
	}
}

static void ajust_key_ptrs (intptr_t offset)
{
	uint i;
	cache_t * c;
	
	for (i = 0; i < cache->cur_len; i++)
	{
		c = &(((cache_t *) cache->data)[i]);
		c->key.str += offset;
	}
}

void cache_store (u_str_t * name, u_str_t * data, void * update_callback)
{
	cache_t * c;
	intptr_t offset;
	
	buf_expand(cache, 1);
	offset = buf_expand(cache_names, name->len);
	
	if (offset)
		ajust_key_ptrs(offset);
	
	c = &(((cache_t *) cache->data)[cache->cur_len - 1]);
	
	memset(c, 0, sizeof(* c));
	
	c->key.str = (uchar *) cache_names->data + (cache_names->cur_len - name->len);
	memcpy(c->key.str, name->str, name->len);
	c->key.len = name->len;
	
	c->value.str = (uchar *) allocator_malloc(data->len);
	memcpy(c->value.str, data->str, data->len);
	c->value.len = data->len;
	
	c->update_callback = (cache_update_cb) update_callback;
	c->time = current_time_sec;
	
	gen_gzipped_value(c);
}

void cache_free (void)
{
	uint i;
	cache_t * c;
	
	for (i = 0; i < cache->cur_len; i++)
	{
		c = &(((cache_t *) cache->data)[i]);
		allocator_free(c->value.str);
	}
	
	buf_free(cache_names);
	buf_free(cache);
}

void cache_create (void)
{
	int res;
	
	cache = buf_create(sizeof(cache_t), HTTP_FILES_CACHE_ELEMS_RESERVED_SIZE);
	cache_names = buf_create(1, 0);
	memset(gz_strm, 0, sizeof(z_stream));
	res = deflateInit2(gz_strm, config.gzip_level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (res != Z_OK)
		peerr(-1, "deflateInit2(): %d", res);
}
