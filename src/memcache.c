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

buf_t * cache;

static z_stream gz_strm[1];
static const char gzip_header[10] = GZIP_HEADER;


static void gen_gzipped_value (cache_t * c)
{
	register int r;
	register uchar * ptr, * p;
	register uint len, l;
	
	ptr = NULL;
	len = 0;
	
	gz_strm->avail_in = c->value.len;
	gz_strm->next_in = c->value.str;
	
	do
	{
		len += GZIP_BUFFER_SIZE;
		ptr = (uchar *) realloc(ptr, len);
		gz_strm->avail_out = GZIP_BUFFER_SIZE;
		gz_strm->next_out = (ptr + len) - GZIP_BUFFER_SIZE;
		r = deflate(gz_strm, Z_FINISH);
		if (r == Z_STREAM_END)
			break;
		if (r != Z_OK)
		{
			err("deflate(): %d", r);
			free(ptr);
			c->gzipped_value.str = NULL;
			c->gzipped_value.len = 0;
			return;
		}
	}
	while (gz_strm->avail_out == 0);
	
	if (c->gzipped_value.len)
		free(c->gzipped_value.str);
	
	len -= gz_strm->avail_out;
	l = len + 18;
	
	p = malloc(l);
	memcpy(p, gzip_header, 10);
	c->gzipped_value.str = p;
	p += 10;
	memcpy(p, ptr, len);
	p += len;
	free(ptr);
	
	_BEGIN_LOCAL_SECTION_
	static __uint32_t gzip_ending[2];
	gzip_ending[0] = crc32(0, c->value.str, c->value.len);
	gzip_ending[1] = c->value.len;
	#if __BYTE_ORDER == __BIG_ENDIAN
	gzip_ending[0] = bswap_32(gzip_ending[0]);
	gzip_ending[1] = bswap_32(gzip_ending[1]);
	#endif
	memcpy(p, gzip_ending, 8);
	_END_LOCAL_SECTION_
	
	c->gzipped_value.len = l;
	
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
			static u_str_t * res;
			
			res = c->update_callback(c->time, name);
			
			if (res != NULL)
			{
				c->value.str = realloc(c->value.str, res->len);
				memcpy(c->value.str, res->str, res->len);
				c->value.len = res->len;
				c->time = current_time_sec;
				gen_gzipped_value(c);
			}
			
			return;
		}
	}
}

void cache_store (u_str_t * name, u_str_t * data, void * update_callback)
{
	cache_t * c;
	buf_expand(cache, 1);
	c = &(((cache_t *) cache->data)[cache->cur_len - 1]);
	c->key.str = (uchar *) malloc(name->len);
	memcpy(c->key.str, name->str, name->len);
	c->key.len = name->len;
	c->value.str = (uchar *) malloc(data->len);
	memcpy(c->value.str, data->str, data->len);
	c->value.len = data->len;
	c->update_callback = (cache_update_cb) update_callback;
	c->time = current_time_sec;
	gen_gzipped_value(c);
}

void cache_free (void)
{
	register uint i;
	register cache_t * c;
	
	for (i = 0; i < cache->cur_len; i++)
	{
		c = &(((cache_t *) cache->data)[i]);
		free(c->key.str);
		free(c->value.str);
		if (c->gzipped_value.len)
			free(c->gzipped_value.str);
	}
	
	buf_free(cache);
}

void cache_create (void)
{
	int res;
	
	cache = buf_create(sizeof(cache_t), HTTP_FILES_CACHE_ELEMS_RESERVED_SIZE);
	memset(gz_strm, 0, sizeof(z_stream));
	res = deflateInit2(gz_strm, config.gzip_level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (res != Z_OK)
		peerr(-1, "deflateInit2(): %d", res);
}
