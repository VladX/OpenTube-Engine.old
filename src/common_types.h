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

#include "libs/zlib.h"

#ifndef  __cplusplus
 #define bool unsigned char
 #define true 1
 #define false 0
#endif

#define CLRF "\r\n"

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long


typedef struct
{
	uint len;
	char * str;
} str_t;

typedef struct
{
	uint len;
	uchar * str;
} u_str_t;

typedef struct
{
	uint cur_len;
	uint real_len;
	uint node_size;
	void ** data;
} pool_t;

typedef struct
{
	void * data;
	bool free;
} frag_pool_elem_t;

typedef struct
{
	uint cur_len;
	uint reserved_len;
	uint real_len;
	uint node_size;
	frag_pool_elem_t * e;
} frag_pool_t;

typedef struct
{
	uint cur_len;
	uint reserved_len;
	uint node_size;
	void * data;
} buf_t;

typedef struct
{
	uchar addr[16];
	time_t time;
	time_t dtime;
	uint req;
} limit_req_t;

typedef struct
{
	int sock;
	time_t time;
} keepalive_sock_t;

typedef struct
{
	u_str_t key;
	u_str_t value;
} assoc_t;

typedef assoc_t header_t;

typedef assoc_t url_arg_t;

typedef assoc_t cookie_t;

typedef assoc_t post_arg_t;

typedef struct
{
	u_str_t data;
	u_str_t key;
	u_str_t name;
	u_str_t type;
} post_file_t;

typedef struct
{
	bool method_get;
	bool method_post;
	bool method_head;
	bool multipart;
	bool urlenc;
	u_str_t uri;
	u_str_t http_version;
	u_str_t path;
	u_str_t query_string;
	header_t * content_type;
	header_t * content_length;
	uint content_length_val;
	struct
	{
		url_arg_t * args;
		uint num;
		buf_t * b;
		buf_t * v;
	} args;
	struct
	{
		cookie_t * cookies;
		uint num;
		buf_t * b;
		buf_t * v;
	} cookies;
	pool_t * p;
} headers_in_t;

typedef struct
{
	u_str_t http_version;
	ulong content_length;
	u_str_t content_type;
	str_t content_range;
	str_t expires;
} headers_out_t;

typedef struct
{
	buf_t * b;
	headers_in_t in;
	headers_out_t out;
	struct
	{
		struct
		{
			post_arg_t * args;
			uint num;
			buf_t * b;
			buf_t * v;
		} post;
		struct
		{
			post_file_t * files;
			uint num;
			buf_t * b;
		} files;
		u_str_t data;
	} body;
	buf_t * out_vec;
	buf_t * out_data;
	int sock;
	bool keepalive;
	time_t keepalive_time;
	struct
	{
		char content_length[16];
		//char * temppath;
		//int temppath_len;
		long writev_total;
		struct iovec * out_vec;
		uint out_vec_len;
		buf_t * filepath;
		int sendfile_fd;
		uint sendfile_last;
		off_t sendfile_offset;
		char dates[60];
		buf_t * gzip_buf;
		z_stream * gzip_stream;
		__uint32_t gzip_ending[2];
		void * func;
	} temp;
} request_t;

typedef buf_t * (* web_func_t) (request_t *, buf_t *);

typedef struct
{
	u_str_t uri;
	web_func_t func;
	bool strict_comparison;
	bool full_match;
} uri_map_t;

typedef struct
{
	const char * user;
	const char * group;
	const char * temp_dir;
	u_str_t document_root;
	u_str_t keepalive_timeout;
	uint keepalive_timeout_val;
	uint keepalive_max_conn_per_client;
	bool gzip;
	uchar gzip_level;
	uint gzip_min_page_size;
	bool limit_req;
	uint limit_rate;
	uint limit_delay;
	bool limit_sim_req;
	uint limit_sim_threshold;
	uchar cache_update;
	u_str_t cache_prefix;
} config_t;
