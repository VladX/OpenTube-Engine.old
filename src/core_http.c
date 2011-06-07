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

#define _GNU_SOURCE
#define __USE_GNU
#define WINVER 0x0501
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "os_stat.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include "sendfile.h"
#include "endianness.h"
#include "libs/zlib.h"
#include "common_functions.h"
#include "core_server.h"
#include "web.h"
#include "templates.h"
#include "error_page.h"
#include "mime.h"
#include "memcache.h"
#include "win32_utils.h"

#define HTTP_OUTPUT_VECTOR_START_SIZE 128


static sem_t wsem[1];
pthread_mutex_t wmutex[1];
static pthread_spinlock_t spin_queue_atomic[1];
static pthread_t * wthreads;
static pqueue_t * wr_queue;
static const char * http_error_message[506];
static const char * http_status_code[506];
static ushort http_error_message_len[506];
static uchar http_status_code_len[506];
static str_t header_server_string;
static const char gzip_header[10] = GZIP_HEADER;


static inline void http_buffer_moved (request_t * r, long offset)
{
	register header_t * hdr;
	register uint i;
	
	r->in.uri.str += offset;
	r->in.http_version.str += offset;
	r->body.data.str += offset;
	
	for (i = 0; i < r->in.p->cur_len; i++)
	{
		hdr = (header_t *) r->in.p->data[i];
		hdr->key.str += offset;
		hdr->value.str += offset;
	}
}

inline void http_append_to_output_buf (request_t * r, const void * pointer, uint len)
{
	struct iovec * iov;
	uint offset;
	
	offset = r->out_vec->cur_len;
	
	buf_expand(r->out_vec, 1);
	
	iov = (struct iovec *) r->out_vec->data;
	
	iov[offset].iov_base = (void *) pointer;
	iov[offset].iov_len = len;
	
	r->temp.writev_total += len;
}

inline void http_append_str (request_t * r, char * str)
{
	http_append_to_output_buf(r, str, strlen((const char *) str));
}

static inline void http_append_headers (request_t * r, ushort code)
{
	http_append_to_output_buf(r, "HTTP/1.1 ", 9);
	http_append_to_output_buf(r, http_status_code[code], http_status_code_len[code]);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, header_server_string.str, header_server_string.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Type: ", 14);
	http_append_to_output_buf(r, r->out.content_type.str, r->out.content_type.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Length: ", 16);
	int_to_str(r->out.content_length, r->temp.content_length, 10);
	http_append_str(r, r->temp.content_length);
	http_append_to_output_buf(r, CLRF, 2);
	if (r->keepalive)
	{
		http_append_to_output_buf(r, "Connection: keep-alive" CLRF "Keep-Alive: timeout=", 44);
		http_append_to_output_buf(r, config.keepalive_timeout.str, config.keepalive_timeout.len);
	}
	else
		http_append_to_output_buf(r, "Connection: close", 17);
	http_append_to_output_buf(r, CLRF, 2);
}

static inline bool http_send (request_t * r)
{
	register ssize_t res;
	register uint size, i, d;
	register struct iovec * data;
	
	data = (struct iovec *) r->out_vec->data;
	
	res = writev(r->sock, data, r->out_vec->cur_len);
	
	if (res == -1 && errno != EAGAIN)
	{
		perr("writev(%d)", r->sock);
		
		return true;
	}
	
	if (res < r->temp.writev_total)
	{
		if (res > 0)
		{
			r->temp.writev_total -= res;
			
			for (i = 0, size = 0; i < r->out_vec->cur_len; i++)
			{
				size += data[i].iov_len;
				
				if (size >= res)
				{
					data += i;
					d = data[0].iov_len - (size - res);
					data[0].iov_base = ((uchar *) data[0].iov_base) + d;
					data[0].iov_len -= d;
					r->temp.out_vec_len = r->out_vec->cur_len - i;
					r->temp.out_vec = data;
					break;
				}
			}
		}
		
		set_epollout_event_mask(r->sock);
		
		return false;
	}
	else
		r->temp.writev_total = 0;
	
	return true;
}

static bool http_redirect (request_t * r, u_str_t * location, bool append_query_string_to_location)
{
	r->out.content_length = 0;
	r->out.content_type.str = (uchar *) HTTP_ERROR_CONTENT_TYPE;
	r->out.content_type.len = HTTP_ERROR_CONTENT_TYPE_LEN;
	http_append_headers(r, 301);
	http_append_to_output_buf(r, "Location: ", 10);
	http_append_to_output_buf(r, location->str, location->len);
	if (append_query_string_to_location && r->in.query_string.len)
	{
		http_append_to_output_buf(r, "?", 1);
		http_append_to_output_buf(r, r->in.query_string.str, r->in.query_string.len);
	}
	http_append_to_output_buf(r, CLRF CLRF, 4);
	
	return http_send(r);
}

static bool http_error (request_t * r, ushort code)
{
	r->out.content_length = http_error_message_len[code];
	r->out.content_type.str = (uchar *) HTTP_ERROR_CONTENT_TYPE;
	r->out.content_type.len = HTTP_ERROR_CONTENT_TYPE_LEN;
	http_append_headers(r, code);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, http_error_message[code], r->out.content_length);
	
	return http_send(r);
}

/* Old version, slower */
/*
static inline uchar http_hex_to_ascii (uchar * c)
{
	uchar l, r;
	
	l = TO_LOWER(c[1]);
	r = ((l >= '0' && l <= '9') ? l - '0' : l - 'a' + 10) * 16;
	l = TO_LOWER(c[2]);
	r += (l >= '0' && l <= '9') ? l - '0' : l - 'a' + 10;
	
	return r;
}
*/

/* New version, faster */
static inline uchar http_hex_to_ascii (uchar * c)
{
	static const uchar h2a_table_1[128] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
	};
	
	static const uchar h2a_table_2[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	
	return (h2a_table_1[c[1] & 127] + h2a_table_2[c[2] & 127]);
}

static inline void http_parse__ (uint * args_num, uchar * raw_str, buf_t * args_buf, buf_t * vals_buf, uchar separator)
{
	uint estimated_args_buffer_size = 0;
	uint estimated_values_buffer_size = 0;
	uint i;
	
	uchar * c = raw_str;
	
	* args_num = 0;
	
	for (;; c++)
	{
		if (* c == separator || * c == '=' || * c == ' ')
			continue;
		for (; * c != '='; c++)
			if (* c == '\0')
				goto _leave_;
			else if (!IS_VALID_URL_KEY_CHARACTER(* c))
				return;
		c++;
		estimated_args_buffer_size++;
		for (; * c != separator; c++, estimated_values_buffer_size++)
			if (* c == '\0')
				goto _leave_;
			else if (!IS_VALID_URL_VALUE_CHARACTER(* c))
				return;
	}
	
	_leave_:
	
	buf_resize(args_buf, estimated_args_buffer_size);
	
	if (estimated_args_buffer_size == 0)
		return;
	
	buf_resize(vals_buf, estimated_values_buffer_size + estimated_args_buffer_size);
	
	_BEGIN_LOCAL_SECTION_
	url_arg_t * arg;
	uchar * p = vals_buf->data;
	
	c = raw_str;
	
	for (i = 0; i < estimated_args_buffer_size; c++)
	{
		if (* c == separator || * c == '=' || * c == ' ')
			continue;
		arg = &(((assoc_t *) args_buf->data)[i]);
		arg->key.str = c;
		for (; * c != '='; c++) {}
		* c = '\0';
		arg->key.len = c - arg->key.str;
		c++;
		arg->value.str = p;
		for (; * c != separator; c++, p++)
		{
			if (* c == '+')
				* p = ' ';
			else if (* c == '%' && * (c + 1) != '\0' && * (c + 2) != '\0')
			{
				* p = http_hex_to_ascii(c);
				c += 2;
			}
			else if (* c == '\0')
				break;
			else
				* p = * c;
		}
		* p = '\0';
		arg->value.len = p - arg->value.str;
		p++;
		i++;
		
		debug_print_3("key: \"%s\", value: \"%s\"", arg->key.str, arg->value.str);
	}
	_END_LOCAL_SECTION_
	
	* args_num = estimated_args_buffer_size;
}

void http_parse_query_string (request_t * r)
{
	http_parse__(&(r->in.args.num), r->in.query_string.str, r->in.args.b, r->in.args.v, '&');
	r->in.args.args = (url_arg_t *) r->in.args.b->data;
}

void http_parse_cookies (request_t * r)
{
	header_t * hdr;
	uint i;
	
	for (i = 0; i < r->in.p->cur_len; i++)
	{
		hdr = (header_t *) r->in.p->data[i];
		if (hdr->key.len == 6 && memcmp(hdr->key.str, "cookie", 6) == 0)
		{
			http_parse__(&(r->in.cookies.num), hdr->value.str, r->in.cookies.b, r->in.cookies.v, ';');
			r->in.cookies.cookies = (cookie_t *) r->in.cookies.b->data;
			break;
		}
	}
}

void http_parse_post (request_t * r)
{
	if (r->in.urlenc)
	{
		http_parse__(&(r->body.post.num), r->body.data.str, r->body.post.b, r->body.post.v, '&');
		r->body.post.args = (post_arg_t *) r->body.post.b->data;
	}
	else
	{
		uchar * c = r->body.data.str;
		uchar * hdr;
		uint i;
		u_str_t content_type, name, filename;
		post_arg_t * arg;
		post_file_t * file;
		
		name.len = 0;
		filename.len = 0;
		content_type.len = 0;
		r->body.post.num = 0;
		r->body.files.num = 0;
		
		if (r->body.data.len < 12)
			return;
		
		for (;;)
		{
			/* --BOUNDARY\r\n */
			for (; !(c[0] == '\r' && c[1] == '\n'); c++)
				if (* c == '\0')
					return;
			c += 2;
			if (* c == '\0')
				return;
			
			/* Headers */
			
			name.str = NULL;
			filename.str = NULL;
			content_type.str = NULL;
			
			for (; !(c[0] == '\r' && c[1] == '\n'); c += 2)
			{
				hdr = c;
				for (; * c != ':'; c++)
				{
					if (* c == '\0')
						return;
					* c = TO_LOWER(* c);
				}
				* c = '\0';
				c++;
				for (; * c == ' '; c++) {}
				if (strcmp((char *) hdr, "content-disposition") == 0)
				{
					for (; !(c[0] == '\r' && c[1] == '\n'); c++)
					{
						if (* c == '\0')
							return;
						
						if (c[0] == ' ' && c[1] == 'n' && c[2] == 'a' && c[3] == 'm' && c[4] == 'e' && c[5] == '=')
						{
							c += 6;
							if (* c == '"')
								c++;
							name.str = c;
							for (; * c != '"' && * c != '&'; c++)
								if (* c == '\0')
									return;
							* c = '\0';
							name.len = c - name.str;
						}
						else if (c[0] == ' ' && c[1] == 'f' && c[2] == 'i' && c[3] == 'l' && c[4] == 'e' &&
						         c[5] == 'n' && c[6] == 'a' && c[7] == 'm' && c[8] == 'e' && c[9] == '=')
						{
							c += 10;
							if (* c == '"')
								c++;
							filename.str = c;
							for (; * c != '"' && * c != '&'; c++)
								if (* c == '\0')
									return;
							* c = '\0';
							filename.len = c - filename.str;
						}
					}
					continue;
				}
				else if (strcmp((char *) hdr, "content-type") == 0)
				{
					content_type.str = c;
					for (; !(c[0] == '\r' && c[1] == '\n'); c++)
						if (* c == '\0')
							return;
					* c = '\0';
					content_type.len = c - content_type.str;
					continue;
				}
				for (; !(c[0] == '\r' && c[1] == '\n'); c++)
					if (* c == '\0')
						return;
			}
			c += 2;
			
			if (name.str)
			{
				if (filename.str)
				{
					buf_resize(r->body.files.b, r->body.files.num + 1);
					r->body.files.files = (post_file_t *) r->body.files.b->data;
					file = &(r->body.files.files[r->body.files.num]);
					file->key.str = name.str;
					file->key.len = name.len;
					file->name.str = filename.str;
					file->name.len = filename.len;
					if (content_type.str)
					{
						file->type.str = content_type.str;
						file->type.len = content_type.len;
					}
					else
					{
						file->type.str = (uchar *) "application/octet-stream";
						file->type.len = 24;
					}
					file->data.str = c;
					for (i = c - r->body.data.str + 4;; c++, i++)
					{
						if (i > r->body.data.len)
							return;
						if (c[0] == '\r' && c[1] == '\n' && c[2] == '-' && c[3] == '-')
							break;
					}
					* c = '\0';
					file->data.len = c - file->data.str;
					r->body.files.num++;
					c += 2;
					
					debug_print_3("file \"%s: %s, %s, %d\"", file->key.str, file->name.str, file->type.str, file->data.len);
				}
				else
				{
					buf_resize(r->body.post.b, r->body.post.num + 1);
					r->body.post.args = (post_arg_t *) r->body.post.b->data;
					arg = &(r->body.post.args[r->body.post.num]);
					arg->key.str = name.str;
					arg->key.len = name.len;
					arg->value.str = c;
					for (i = c - r->body.data.str + 4;; c++, i++)
					{
						if (i > r->body.data.len)
							return;
						if (c[0] == '\r' && c[1] == '\n' && c[2] == '-' && c[3] == '-')
							break;
					}
					* c = '\0';
					arg->value.len = c - arg->value.str;
					r->body.post.num++;
					c += 2;
					
					debug_print_3("form \"%s: %s\"", arg->key.str, arg->value.str);
				}
			}
		}
	}
}

static bool http_divide_uri (request_t * r)
{
	register uchar * c, * d;
	register uint i;
	
	if (r->in.uri.len > HTTP_PATH_PREALLOC)
		r->in.path.str = (uchar *) realloc(r->in.path.str, r->in.uri.len);
	c = r->in.path.str;
	d = r->in.uri.str;
	
	* (c++) = '/';
	
	for (i = 1; i < r->in.uri.len && d[i] != '?'; i++)
	{
		if (d[i] == '%' && i < r->in.uri.len - 2)
		{
			* c = http_hex_to_ascii(d + i);
			i += 2;
			if (* c != '/' && * c != '.')
				c++;
		}
		else if (d[i] == '.' && d[i + 1] == '.' && d[i + 2] == '/')
			i += 2;
		else if (IS_VALID_PATH_CHARACTER(d[i]))
		{
			* c = d[i];
			c++;
		}
		else
			return false;
	}
	
	* c = '\0';
	r->in.path.len = c - r->in.path.str;
	
	debug_print_3("r->in.path.str: \"%s\"", r->in.path.str);
	
	r->in.query_string.str = d + i;
	
	if (r->in.query_string.str[0] == '?')
	{
		r->in.query_string.str++;
		r->in.query_string.len = r->in.uri.len - i - 1;
	}
	else
		r->in.query_string.len = 0;
	
	return true;
}

static const char * days_three_sym[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char * months_three_sym[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static inline void http_rfc822_date (char * str, struct tm * ctime)
{
	sprintf(str, "%s, %02d %s %04d %02d:%02d:%02d GMT", days_three_sym[ctime->tm_wday], ctime->tm_mday, months_three_sym[ctime->tm_mon], ctime->tm_year + 1900, ctime->tm_hour, ctime->tm_min, ctime->tm_sec);
}

static inline void http_set_mime_type (request_t * r)
{
	register uchar * fileext;
	register uint i;
	
	fileext = r->in.path.str + r->in.path.len;
	
	for (i = r->in.path.len - 1; i > 0; i--)
		if (r->in.path.str[i] == '.')
		{
			fileext = r->in.path.str + i;
			break;
		}
	
	for (i = 0; * (http_mime_types[i]); i += 3)
	{
		if (strcmp((const char *) http_mime_types[i], (const char *) fileext) == 0)
			break;
	}
	
	r->out.content_type.str = (uchar *) http_mime_types[i + 1];
	r->out.content_type.len = * (http_mime_types[i + 2]);
}

static inline bool http_send_file (request_t * r, const char * filepath)
{
	register uint i, it;
	static bool ret;
	static struct stat st;
	register int fd, t;
	
	if (* filepath == '\0')
		return http_error(r, 403);
	
	fd = open(filepath, O_RDONLY);
	
	if (fd == -1)
	{
		if (io_errno == ENOENT || errno == ENOTDIR)
			return http_error(r, 404);
		
		if (io_errno == EACCES)
			return http_error(r, 403);
		
		perr("Access to file \"%s\"", filepath);
		
		return http_error(r, 500);
	}
	
	if (fstat(fd, &st) == -1)
	{
		perr("fstat(): %d", -1);
		ret = http_error(r, 500);
		close(fd);
		
		return ret;
	}
	
	if (!(S_ISREG(st.st_mode)))
	{
		ret = http_error(r, 403);
		close(fd);
		
		return ret;
	}
	
	http_set_mime_type(r);
	
	_BEGIN_LOCAL_SECTION_
	register ushort code;
	#ifndef _WIN
	register ssize_t res;
	#endif
	register header_t * hdr;
	static struct tm c_time, m_time;
	static time_t curtime;
	register char * rfc822_date_str;
	
	code = 200;
	rfc822_date_str = r->temp.dates;
	
	r->temp.sendfile_last = st.st_size;
	r->temp.sendfile_offset = 0;
	
	gmtime_r(&(st.st_mtime), &m_time);
	http_rfc822_date(rfc822_date_str, &m_time);
	
	for (i = 0; i < r->in.p->cur_len; i++)
	{
		hdr = (header_t *) r->in.p->data[i];
		if (hdr->key.len == 17 && hdr->value.len == 29 && memcmp(hdr->key.str, "if-modified-since", 17) == 0 && memcmp(hdr->value.str, rfc822_date_str, 29) == 0)
		{
			code = 304;
			break;
		}
		else if (hdr->key.len == 5 && memcmp(hdr->key.str, "range", 5) == 0 && hdr->value.len > 6 && memcmp(hdr->value.str, "bytes=", 6) == 0)
		{
			for (it = 6; it < hdr->value.len; it++)
				if (hdr->value.str[it] == '-')
				{
					hdr->value.str[it] = '\0';
					if (hdr->value.str[it + 1])
					{
						t = atoi((const char *) hdr->value.str + it + 1);
						r->temp.sendfile_last = max(0, t);
						r->temp.sendfile_last++;
						r->temp.sendfile_last = min(st.st_size, r->temp.sendfile_last);
					}
					break;
				}
			r->temp.sendfile_offset = atoi((const char *) hdr->value.str + 6);
			if (r->temp.sendfile_offset >= st.st_size)
			{
				ret = http_error(r, 416);
				close(fd);
				
				return ret;
			}
			if (r->temp.sendfile_offset > r->temp.sendfile_last)
				r->temp.sendfile_last = r->temp.sendfile_offset + 1;
			code = 206;
		}
	}
	
	r->out.content_length = r->temp.sendfile_last - r->temp.sendfile_offset;
	
	http_append_headers(r, code);
	
	http_append_to_output_buf(r, "Last-Modified: ", 15);
	http_append_to_output_buf(r, rfc822_date_str, 29);
	http_append_to_output_buf(r, CLRF, 2);
	
	curtime = current_time_sec;
	gmtime_r(&curtime, &c_time);
	http_rfc822_date(rfc822_date_str + 30, &c_time);
	http_append_to_output_buf(r, "Date: ", 6);
	http_append_to_output_buf(r, rfc822_date_str + 30, 29);
	http_append_to_output_buf(r, CLRF, 2);
	
	curtime += HTTP_STATIC_EXPIRES;
	gmtime_r(&curtime, &c_time);
	http_rfc822_date(r->out.expires.str, &c_time);
	http_append_to_output_buf(r, "Expires: ", 9);
	http_append_to_output_buf(r, r->out.expires.str, r->out.expires.len);
	http_append_to_output_buf(r, CLRF, 2);
	
	if (code == 206)
	{
		sprintf(r->out.content_range.str, "Content-Range: bytes %u-%u/%u" CLRF, (uint) r->temp.sendfile_offset, r->temp.sendfile_last - 1, (uint) st.st_size);
		http_append_str(r, r->out.content_range.str);
	}
	
	http_append_to_output_buf(r, "Accept-Ranges: bytes" CLRF CLRF, 24);
	
	if (code == 304)
	{
		ret = http_send(r);
		close(fd);
		
		return ret;
	}
	
	if (http_send(r) == false)
	{
		r->temp.sendfile_fd = fd;
		
		return false;
	}
	
	#ifndef _WIN
	#ifdef _BSD
	static off_t sbytes;
	sbytes = 0;
	res = sendfile(fd, r->sock, r->temp.sendfile_offset, (size_t) r->out.content_length, NULL, &sbytes, 0);
	r->temp.sendfile_offset += sbytes;
	#else
	res = sendfile(r->sock, fd, &(r->temp.sendfile_offset), (size_t) r->out.content_length);
	#endif
	
	if (res == -1 && errno != EAGAIN)
	{
		perr("sendfile(): %d", (int) res);
		
		return true;
	}
	#ifdef _BSD
	if (sbytes < r->out.content_length)
	#else
	if (res < r->out.content_length)
	#endif
	{
		r->temp.sendfile_fd = fd;
		set_epollout_event_mask(r->sock);
		
		return false;
	}
	
	close(fd);
	_END_LOCAL_SECTION_
	
	return true;
	#else
	_BEGIN_LOCAL_SECTION_
	static OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.Offset = r->temp.sendfile_offset;
	ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!TransmitFile(r->sock, (HANDLE) _get_osfhandle(fd), r->out.content_length, 0, &ov, NULL, TF_USE_SYSTEM_THREAD) && io_errno != WSA_IO_PENDING)
		perr("TransmitFile(%d)", r->sock);
	
	r->temp.TransmitFileHandle = fd;
	r->temp.EventHandle = ov.hEvent;
	r->temp.WaitHandle = NULL;
	(void) RegisterWaitForSingleObject(&(r->temp.WaitHandle), ov.hEvent, (WAITORTIMERCALLBACK) win32_transmit_complete_cb, r, INFINITE, WT_EXECUTEINPERSISTENTTHREAD | WT_EXECUTEONLYONCE);
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
	
	return false;
	#endif
}

void run_init_callbacks (void);

static void * http_pass_to_handlers_routine (void * ptr)
{
	uint i = 0;
	request_t * r = NULL;
	int res;
	buf_t * buf;
	bool accept_gzip;
	struct tm c_time;
	time_t curtime;
	ushort code;
	
	extern threadsafe jmp_buf web_exceptions_jmpbuf;
	extern threadsafe request_t * thread_request;
	extern threadsafe volatile bool thread_allow_compression;
	
	pthread_spin_lock(spin_queue_atomic);
	tpl_init();
	run_init_callbacks();
	pthread_spin_unlock(spin_queue_atomic);
	
	code = (ushort) setjmp(web_exceptions_jmpbuf);
	
	if (code)
	{
		pthread_mutex_lock(wmutex);
		debug_print_3("exception code %u", (uint) code);
		if (http_error(thread_request, code))
			end_request(thread_request);
		pthread_mutex_unlock(wmutex);
	}
	
	for (;;)
	{
		_start_from_the_beginning_:
		
		accept_gzip = false;
		
		sem_wait(wsem);
		
		pthread_spin_lock(spin_queue_atomic);
		r = pqueue_fetch(wr_queue);
		pthread_spin_unlock(spin_queue_atomic);
		
		r->out.content_type.str = (uchar *) "text/html";
		r->out.content_type.len = 9;
		
		web_setup_global_buffer(r);
		
		buf = ((web_func_t) r->temp.func)();
		
		/* Condition r->sock == -1 means, that client closed connection while handler is executed */
		if (r->sock == -1)
			goto _start_from_the_beginning_;
		
		if (config.gzip && thread_allow_compression && buf->cur_len > config.gzip_min_page_size)
		{
			header_t * hdr;
			
			for (i = 0; i < r->in.p->cur_len; i++)
			{
				hdr = (header_t *) r->in.p->data[i];
				if (hdr->key.len == 15 && memcmp(hdr->key.str, "accept-encoding", 15) == 0)
				{
					if (strstr((const char *) hdr->value.str, "gzip") != NULL)
						accept_gzip = true;
				}
			}
		}
		
		if (accept_gzip)
		{
			register z_stream * z = r->temp.gzip_stream;
			buf_expand(r->temp.gzip_buf, buf->cur_len);
			z->avail_in = buf->cur_len;
			z->next_in = buf->data;
			z->avail_out = r->temp.gzip_buf->cur_len;
			z->next_out = r->temp.gzip_buf->data;
			
			do
			{
				res = deflate(z, Z_FINISH);
				
				if (res == Z_STREAM_END)
					break;
				
				if (res == Z_OK)
				{
					buf_expand(r->temp.gzip_buf, 256);
					z->avail_out = 256;
					z->next_out = (uchar *) r->temp.gzip_buf->data + r->temp.gzip_buf->cur_len - 256;
					
					continue;
				}
				else
				{
					err("deflate(): %d", res);
					
					pthread_mutex_lock(wmutex);
					if (http_error(r, 500))
						end_request(r);
					pthread_mutex_unlock(wmutex);
					
					goto _start_from_the_beginning_;
				}
			}
			while (z->avail_out == 0);
			
			i = r->temp.gzip_buf->cur_len - z->avail_out;
			r->out.content_length = i + 18;
			
			(void) deflateReset(z);
			
			http_append_headers(r, 200);
			http_append_to_output_buf(r, "Content-Encoding: gzip" CLRF "Vary: Accept-Encoding" CLRF, 47);
		}
		else
		{
			r->out.content_length = buf->cur_len;
			http_append_headers(r, 200);
		}
		
		http_append_to_output_buf(r, "Cache-Control: no-cache, must-revalidate" CLRF "Pragma: no-cache" CLRF, 60);
		curtime = current_time_sec;
		(void) gmtime_r(&curtime, &c_time);
		http_rfc822_date(r->temp.dates, &c_time);
		http_append_to_output_buf(r, "Date: ", 6);
		http_append_to_output_buf(r, r->temp.dates, 29);
		http_append_to_output_buf(r, CLRF CLRF, 4);
		
		if (accept_gzip)
		{
			http_append_to_output_buf(r, gzip_header, 10);
			http_append_to_output_buf(r, r->temp.gzip_buf->data, i);
			r->temp.gzip_ending[0] = crc32(0, buf->data, buf->cur_len);
			r->temp.gzip_ending[1] = buf->cur_len;
			#if __BYTE_ORDER == __BIG_ENDIAN
			r->temp.gzip_ending[0] = bswap_32(r->temp.gzip_ending[0]);
			r->temp.gzip_ending[1] = bswap_32(r->temp.gzip_ending[1]);
			#endif
			http_append_to_output_buf(r, r->temp.gzip_ending, 8);
		}
		else
			http_append_to_output_buf(r, buf->data, r->out.content_length);
		
		pthread_mutex_lock(wmutex);
		if (r->sock != -1 && http_send(r))
			end_request(r);
		pthread_mutex_unlock(wmutex);
	}
	
	return NULL;
}

static bool http_response (request_t * r)
{
	register uint i;
	register uri_map_t * m;
	
	m = (uri_map_t *) uri_map->data;
	
	if (!http_divide_uri(r))
		return http_error(r, 400);
	
	for (i = 0; i < uri_map->cur_len; i++)
	{
		if (m[i].full_match)
		{
			if (m[i].strict_comparison || r->in.path.str[r->in.path.len - 1] == '/')
			{
				if (r->in.path.len != m[i].uri.len || memcmp(r->in.path.str, m[i].uri.str, r->in.path.len) != 0)
					continue;
			}
			else if (r->in.path.len == m[i].uri.len - 1 && memcmp(r->in.path.str, m[i].uri.str, r->in.path.len) == 0)
				return http_redirect(r, &(m[i].uri), true);
			else
				continue;
		}
		else
		{
			if (strncmp((char *) r->in.path.str, (char *) m[i].uri.str, m[i].uri.len) != 0)
				continue;
		}
		
		disable_events_for_socket(r->sock);
		
		r->temp.func = m[i].func;
		
		pthread_spin_lock(spin_queue_atomic);
		pqueue_push(wr_queue, r);
		pthread_spin_unlock(spin_queue_atomic);
		sem_post(wsem);
		
		return false;
	}
	
	_BEGIN_LOCAL_SECTION_
	register u_str_t * cached_content;
	register header_t * hdr;
	register bool accept_gzip;
	
	cached_content = NULL;
	accept_gzip = false;
	
	if (r->in.path.len > config.cache_prefix.len && memcmp(r->in.path.str, config.cache_prefix.str, config.cache_prefix.len) == 0)
	{
		if (config.cache_update == 1)
			cache_update(&(r->in.path));
		
		if (config.gzip)
			for (i = 0; i < r->in.p->cur_len; i++)
			{
				hdr = (header_t *) r->in.p->data[i];
				if (hdr->key.len == 15 && memcmp(hdr->key.str, "accept-encoding", 15) == 0)
				{
					if (strstr((const char *) hdr->value.str, "gzip") != NULL)
						accept_gzip = true;
				}
			}
		
		cached_content = cache_find(&(r->in.path), accept_gzip);
	}
	
	if (cached_content != NULL)
	{
		http_set_mime_type(r);
		
		_BEGIN_LOCAL_SECTION_
		register ushort code;
		
		code = 200;
		
		if (config.cache_update == 0)
			for (i = 0; i < r->in.p->cur_len; i++)
			{
				hdr = (header_t *) r->in.p->data[i];
				if (hdr->key.len == 17 && memcmp(hdr->key.str, "if-modified-since", 17) == 0)
				{
					code = 304;
					break;
				}
			}
		
		r->out.content_length = cached_content->len;
		
		http_append_headers(r, code);
		
		if (accept_gzip)
			http_append_to_output_buf(r, "Content-Encoding: gzip" CLRF "Vary: Accept-Encoding" CLRF, 47);
		
		_BEGIN_LOCAL_SECTION_
		time_t curtime = current_time_sec;
		struct tm c_time;
		(void) gmtime_r(&curtime, &c_time);
		http_rfc822_date(r->temp.dates, &c_time);
		
		http_append_to_output_buf(r, "Last-Modified: ", 15);
		http_append_to_output_buf(r, r->temp.dates, 29);
		http_append_to_output_buf(r, CLRF, 2);
		
		http_append_to_output_buf(r, "Date: ", 6);
		http_append_to_output_buf(r, r->temp.dates, 29);
		http_append_to_output_buf(r, CLRF, 2);
		
		curtime += HTTP_STATIC_EXPIRES;
		(void) gmtime_r(&curtime, &c_time);
		
		http_rfc822_date(r->out.expires.str, &c_time);
		http_append_to_output_buf(r, "Expires: ", 9);
		http_append_to_output_buf(r, r->out.expires.str, r->out.expires.len);
		http_append_to_output_buf(r, CLRF CLRF, 4);
		_END_LOCAL_SECTION_
		
		if (code != 304)
			http_append_to_output_buf(r, cached_content->str, cached_content->len);
		
		_END_LOCAL_SECTION_
		
		return http_send(r);
	}
	_END_LOCAL_SECTION_
	
	return http_send_file(r, (const char *) r->in.path.str + 1);
}

static ushort http_parse_headers (request_t * r)
{
	register uchar * p;
	register header_t * header;
	register uchar * key, * val;
	
	p = (uchar *) r->b->data;
	
	r->in.method_get = (r->in.method_post = (r->in.method_head = false));
	
	if (p[0] == 'G' && p[1] == 'E' && p[2] == 'T' && p[3] == ' ')
	{
		r->in.method_get = true;
		p += 4;
	}
	else if (p[0] == 'P' && p[1] == 'O' && p[2] == 'S' && p[3] == 'T' && p[4] == ' ')
	{
		r->in.method_post = true;
		p += 5;
	}
	else if (p[0] == 'H' && p[1] == 'E' && p[2] == 'A' && p[3] == 'D' && p[4] == ' ')
	{
		r->in.method_head = true;
		p += 5;
	}
	else
		return 501;
	
	if (* p != '/')
		return 400;
		
	r->in.uri.str = p;
	
	for (; * p != ' '; p++)
		r->in.uri.len++;
	
	r->in.uri.str[r->in.uri.len] = '\0';
	
	debug_print_2("r->in.uri.str: \"%s\"", r->in.uri.str);
	
	if (p[1] != 'H' || p[2] != 'T' || p[3] != 'T' || p[4] != 'P' || p[5] != '/')
		return 400;
	
	p += 6;
	r->in.http_version.str = p;
	r->in.http_version.len = 0;
	
	for (; * p != '\r'; p++)
		r->in.http_version.len++;
	
	r->in.http_version.str[r->in.http_version.len] = '\0';
	
	p += 2;
	
	while (* p != 1)
	{
		header = (header_t *) pool_alloc(r->in.p);
		
		for (key = p; * p != ':'; p++)
		{
			if (!IS_SYM(* p))
				return 400;
			
			* p = TO_LOWER(* p);
		}
		
		header->key.str = key;
		header->key.len = p - key;
		header->key.str[header->key.len] = '\0';
		
		p++;
		
		while (* p == ' ')
			p++;
		
		for (val = p; p[0] != '\r' || p[1] != '\n'; p++) {}
		
		header->value.str = val;
		header->value.len = p - val;
		header->value.str[header->value.len] = '\0';
		
		if (
		header->key.len == 10 && strcmp((const char *) header->key.str, "connection") == 0 && header->value.len > 9 &&
		/* For performance reasons we compare only first and last letter of connection header value (looking for "[k]eep-aliv[e]"), not full string */
		(header->value.str[0] == 'k' || header->value.str[0] == 'K') && (header->value.str[9] == 'e' || header->value.str[9] == 'E')
		)
			r->keepalive = new_keepalive_socket(r->sock);
		
		debug_print_3("header \"%s: %s\"", header->key.str, header->value.str);
		
		p += 2;
	}
	
	return 0;
}

bool http_serve_client (request_t * request)
{
	static ushort code;
	register int r;
	register uint i;
	register header_t * hdr;
	static uchar * buf;
	static long offset;
	
	offset = buf_expand(request->b, HTTP_RECV_BUFFER);
	if (offset)
		http_buffer_moved(request, offset);
	buf = (uchar *) request->b->data + request->b->cur_len - HTTP_RECV_BUFFER;
	
	while ((r = recv(request->sock, (void *) buf, HTTP_RECV_BUFFER, MSG_DONTWAIT)) > 0)
	{
		request->b->cur_len -= HTTP_RECV_BUFFER - r;
		
		if (request->in.uri.len == 0)
		{
			for (i = 4; i < r; i++)
			{
				if (buf[i] == '\n' && buf[i - 1] == '\r' && buf[i - 2] == '\n' && buf[i - 3] == '\r')
				{
					if (request->b->cur_len > HTTP_MAX_HEADERS_SIZE)
						return http_error(request, 414);
					
					buf[i - 1] = 1;
					code = http_parse_headers(request);
					
					if (code)
						return http_error(request, code);
					
					if (!(request->in.method_post))
						return http_response(request);
					else
					{
						buf += i + 1;
						request->body.data.str = buf;
						request->body.data.len = r - (i + 1);
						
						for (i = 0; i < request->in.p->cur_len; i++)
						{
							hdr = (header_t *) request->in.p->data[i];
							if (hdr->key.len == 12 && request->in.content_type == NULL && memcmp(hdr->key.str, "content-type", 12) == 0)
								request->in.content_type = hdr;
							else if (hdr->key.len == 14 && request->in.content_length == NULL && memcmp(hdr->key.str, "content-length", 14) == 0)
								request->in.content_length = hdr;
						}
						
						if (request->in.content_type == NULL)
							return http_error(request, 400);
						
						if (request->in.content_length == NULL)
							return http_error(request, 411);
						
						request->in.multipart = (request->in.urlenc = false);
						if (request->in.content_type->value.len == 33 && memcmp(request->in.content_type->value.str, "application/x-www-form-urlencoded", 33) == 0)
							request->in.urlenc = true;
						else if (request->in.content_type->value.len > 18 && memcmp(request->in.content_type->value.str, "multipart/form-data", 19) == 0)
							request->in.multipart = true;
						else
							return http_error(request, 415);
						
						request->in.content_length_val = (uint) atoi((const char *) request->in.content_length->value.str);
						if (request->in.content_length_val > HTTP_MAX_REQUEST_BODY_SIZE)
							return http_error(request, 413);
						
						goto _post_;
					}
				}
			}
			
			if (request->b->cur_len > HTTP_MAX_HEADERS_SIZE)
				return http_error(request, 414);
			
			goto _loop_end_;
		}
		
		if (!(request->in.method_post))
			return false;
		
		request->body.data.len += r;
		
		_post_:
		
		if (request->body.data.len >= request->in.content_length_val)
		{
			offset = buf_expand(request->b, 1);
			if (offset)
				http_buffer_moved(request, offset);
			request->body.data.str[request->body.data.len] = '\0';
			
			return http_response(request);
		}
		
		_loop_end_:
		
		if (r < HTTP_RECV_BUFFER)
			return false;
		
		offset = buf_expand(request->b, HTTP_RECV_BUFFER);
		if (offset)
			http_buffer_moved(request, offset);
		buf = (uchar *) request->b->data + request->b->cur_len - HTTP_RECV_BUFFER;
	}
	
	if (r == -1 && errno == EAGAIN)
	{
		request->b->cur_len -= HTTP_RECV_BUFFER;
		
		return false;
	}
	
	#if DEBUG_LEVEL
	if (r == -1)
		perr("recv(%d)", request->sock);
	#endif
	
	request->keepalive = false;
	remove_keepalive_socket(request->sock);
	
	return true;
}

void http_cleanup (request_t * r)
{
	if (r->keepalive)
		set_epollin_event_mask(r->sock);
	else
	{
		socket_close(r->sock);
		socket_del_from_event_list(r->sock);
		debug_print_2("close(): %d", r->sock);
	}
	
	r->sock = -1;
	r->keepalive = false;
	
	r->in.uri.len = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	
	r->temp.writev_total = 0;
	r->temp.sendfile_fd = -1;
	
	if (config.gzip)
		buf_free(r->temp.gzip_buf);
	
	buf_free(r->out_vec);
	buf_free(r->b);
	
	pool_free(r->in.p, HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
}

static void http_init_constants (void);

static bool http_prepare_once_flag = false;

static void http_prepare_once (void)
{
	int i;
	
	if (http_prepare_once_flag)
		return;
	
	header_server_string.str = "Server: " SERVER_STRING;
	header_server_string.len = strlen(header_server_string.str);
	
	http_init_constants();
	cache_create();
	
	wr_queue = pqueue_create(config.prealloc_request_structures * 2);
	
	sem_init(wsem, 0, 0);
	pthread_mutex_init(wmutex, NULL);
	pthread_spin_init(spin_queue_atomic, PTHREAD_PROCESS_PRIVATE);
	
	wthreads = (pthread_t *) malloc(config.worker_threads * sizeof(pthread_t));
	
	for (i = 0; i < config.worker_threads; i++)
		pthread_create(&(wthreads[i]), NULL, http_pass_to_handlers_routine, NULL);
	
	http_prepare_once_flag = true;
	
	if (chdir((char *) config.document_root.str) == -1)
		peerr(-1, "chdir(%s): ", config.document_root.str);
}

void http_prepare (request_t * r, bool save_space)
{
	int res;
	
	http_prepare_once();
	
	r->sock = -1;
	r->keepalive = false;
	r->keepalive_time = 1;
	r->in.method_get = (r->in.method_post = (r->in.method_head = false));
	r->in.uri.len = 0;
	r->in.path.str = (uchar *) malloc(HTTP_PATH_PREALLOC);
	r->in.path.len = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	r->temp.writev_total = 0;
	r->temp.sendfile_fd = -1;
	
	r->out.content_range.str = (char *) malloc(64);
	r->out.content_range.len = 0;
	r->out.expires.str = (char *) malloc(29);
	r->out.expires.len = 29;
	
	r->out_data = buf_create(1, WEB_DATA_BUFFER_RESERVED_SIZE);
	
	if (config.gzip)
	{
		r->temp.gzip_buf = buf_create(1, HTTP_GZIP_BUFFER_RESERVED_SIZE);
		r->temp.gzip_stream = (z_stream *) malloc(sizeof(z_stream));
		memset(r->temp.gzip_stream, 0, sizeof(z_stream));
		res = deflateInit2(r->temp.gzip_stream, config.gzip_level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		if (res != Z_OK)
			peerr(-1, "deflateInit2(): %d", res);
	}
	
	r->out_vec = buf_create(sizeof(struct iovec), HTTP_OUTPUT_VECTOR_START_SIZE);
	
	if (save_space)
		r->b = buf_create(1, 1);
	else
		r->b = buf_create(1, HTTP_BUFFER_RESERVED_SIZE);
	
	r->in.p = pool_create(sizeof(header_t), HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
	
	r->in.args.b = buf_create(sizeof(url_arg_t), HTTP_URL_ARGS_BUFFER_RESERVED_SIZE);
	
	if (save_space)
		r->in.args.v = buf_create(1, 1);
	else
		r->in.args.v = buf_create(1, HTTP_URL_VALS_BUFFER_RESERVED_SIZE);
	
	r->in.cookies.b = buf_create(sizeof(cookie_t), HTTP_COOKIES_ARGS_BUFFER_RESERVED_SIZE);
	
	if (save_space)
		r->in.cookies.v = buf_create(1, 1);
	else
		r->in.cookies.v = buf_create(1, HTTP_COOKIES_VALS_BUFFER_RESERVED_SIZE);
	
	r->body.post.b = buf_create(sizeof(post_arg_t), HTTP_POST_ARGS_BUFFER_RESERVED_SIZE);
	
	if (save_space)
		r->body.post.v = buf_create(1, 1);
	else
		r->body.post.v = buf_create(1, HTTP_POST_VALS_BUFFER_RESERVED_SIZE);
	
	r->body.files.b = buf_create(sizeof(post_file_t), HTTP_POST_FILES_BUFFER_RESERVED_SIZE);
}

static void http_init_constants (void)
{
	/* 2xx */
	http_status_code[200] = "200 OK";
	http_status_code_len[200] = strlen(http_status_code[200]);
	http_status_code[206] = "206 Partial Content";
	http_status_code_len[206] = strlen(http_status_code[206]);
	/* 3xx */
	http_status_code[301] = "301 Moved Permanently";
	http_status_code_len[301] = strlen(http_status_code[301]);
	http_status_code[304] = "304 Not Modified";
	http_status_code_len[304] = strlen(http_status_code[304]);
	/* 4xx */
	http_error_message[400] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "400 - Bad Request"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "400"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Bad Request"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[400] = strlen(http_error_message[400]);
	http_status_code[400] = "400 Bad Request";
	http_status_code_len[400] = strlen(http_status_code[400]);
	http_error_message[401] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "401 - Unauthorized"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "401"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Unauthorized"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[401] = strlen(http_error_message[401]);
	http_status_code[401] = "401 Unauthorized";
	http_status_code_len[401] = strlen(http_status_code[401]);
	http_error_message[402] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "402 - Payment Required"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "402"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Payment Required"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[402] = strlen(http_error_message[402]);
	http_status_code[402] = "402 Payment Required";
	http_status_code_len[402] = strlen(http_status_code[402]);
	http_error_message[403] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "403 - Forbidden"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "403"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Forbidden"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[403] = strlen(http_error_message[403]);
	http_status_code[403] = "403 Forbidden";
	http_status_code_len[403] = strlen(http_status_code[403]);
	http_error_message[404] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "404 - Not Found"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "404"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Not Found"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[404] = strlen(http_error_message[404]);
	http_status_code[404] = "404 Not Found";
	http_status_code_len[404] = strlen(http_status_code[404]);
	http_error_message[405] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "405 - Method Not Allowed"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "405"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Method Not Allowed"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[405] = strlen(http_error_message[405]);
	http_status_code[405] = "405 Method Not Allowed";
	http_status_code_len[405] = strlen(http_status_code[405]);
	http_error_message[406] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "406 - Not Acceptable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "406"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Not Acceptable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[406] = strlen(http_error_message[406]);
	http_status_code[406] = "406 Not Acceptable";
	http_status_code_len[406] = strlen(http_status_code[406]);
	http_error_message[408] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "408 - Request Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "408"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Request Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[408] = strlen(http_error_message[408]);
	http_status_code[408] = "408 Request Timeout";
	http_status_code_len[408] = strlen(http_status_code[408]);
	http_error_message[411] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "411 - Length Required"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "411"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Length Required"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[411] = strlen(http_error_message[411]);
	http_status_code[411] = "411 Length Required";
	http_status_code_len[411] = strlen(http_status_code[411]);
	http_error_message[413] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "413 - Request Entity Too Large"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "413"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Request Entity Too Large"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[413] = strlen(http_error_message[413]);
	http_status_code[413] = "413 Request Entity Too Large";
	http_status_code_len[413] = strlen(http_status_code[413]);
	http_error_message[414] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "414 - Request-URL Too Long"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "414"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Request-URL Too Long"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[414] = strlen(http_error_message[414]);
	http_status_code[414] = "414 Request-URL Too Long";
	http_status_code_len[414] = strlen(http_status_code[414]);
	http_error_message[415] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "415 - Unsupported Media Type"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "415"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Unsupported Media Type"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[415] = strlen(http_error_message[415]);
	http_status_code[415] = "415 Unsupported Media Type";
	http_status_code_len[415] = strlen(http_status_code[415]);
	http_error_message[416] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "416 - Requested Range Not Satisfiable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "416"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Requested Range Not Satisfiable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[416] = strlen(http_error_message[416]);
	http_status_code[416] = "416 Requested Range Not Satisfiable";
	http_status_code_len[416] = strlen(http_status_code[416]);
	/* 5xx */
	http_error_message[500] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "500 - Internal Server Error"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "500"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Internal Server Error"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[500] = strlen(http_error_message[500]);
	http_status_code[500] = "500 Internal Server Error";
	http_status_code_len[500] = strlen(http_status_code[500]);
	http_error_message[501] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "501 - Not Implemented"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "501"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Not Implemented"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[501] = strlen(http_error_message[501]);
	http_status_code[501] = "501 Not Implemented";
	http_status_code_len[501] = strlen(http_status_code[501]);
	http_error_message[502] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "502 - Bad Gateway"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "502"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Bad Gateway"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[502] = strlen(http_error_message[502]);
	http_status_code[502] = "502 Bad Gateway";
	http_status_code_len[502] = strlen(http_status_code[502]);
	http_error_message[503] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "503 - Service Unavailable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "503"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Service Unavailable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[503] = strlen(http_error_message[503]);
	http_status_code[503] = "503 Service Unavailable";
	http_status_code_len[503] = strlen(http_status_code[503]);
	http_error_message[504] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "504 - Gateway Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "504"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "Gateway Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[504] = strlen(http_error_message[504]);
	http_status_code[504] = "504 Gateway Timeout";
	http_status_code_len[504] = strlen(http_status_code[504]);
	http_error_message[505] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "505 - HTTP Version Not Supported"
	                          HTTP_HTML_PAGE_TEMPLATE_CON1
	                          "505"
	                          HTTP_HTML_PAGE_TEMPLATE_CON2
	                          "HTTP Version Not Supported"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[505] = strlen(http_error_message[505]);
	http_status_code[505] = "505 HTTP Version Not Supported";
	http_status_code_len[505] = strlen(http_status_code[505]);
}
