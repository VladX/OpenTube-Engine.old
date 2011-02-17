#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "libs/zlib.h"
#include "common_functions.h"
#include "core_server.h"
#include "error_page.h"
#include "mime.h"


#define HTTP_OUTPUT_VECTOR_START_SIZE 128


static const char * http_error_message[506];
static const char * http_status_code[506];
static ushort http_error_message_len[506];
static uchar http_status_code_len[506];
static str_t header_server_string;
static const char gzip_header[10] = {0x1F, 0x8B, Z_DEFLATED, 0, 0, 0, 0, 0, 0, 3};


inline void http_append_to_output_buf (request_t * r, void * pointer, uint len)
{
	struct iovec * iov;
	uint offset;
	
	offset = r->out_vec->cur_len;
	
	buf_expand(r->out_vec, 1);
	
	iov = (struct iovec *) r->out_vec->data;
	
	iov[offset].iov_base = pointer;
	iov[offset].iov_len = len;
	
	r->temp.writev_total += len;
}

inline void http_append_str (request_t * r, char * str)
{
	http_append_to_output_buf(r, str, strlen((const char *) str));
}

static inline void http_append_headers (request_t * r, ushort code)
{
	char temp[16];
	
	http_append_to_output_buf(r, "HTTP/1.1 ", 9);
	http_append_to_output_buf(r, (void *) http_status_code[code], http_status_code_len[code]);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, header_server_string.str, header_server_string.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Type: ", 14);
	http_append_to_output_buf(r, r->out.content_type.str, r->out.content_type.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Length: ", 16);
	int_to_str(r->out.content_length, temp, 10);
	http_append_str(r, temp);
	http_append_to_output_buf(r, CLRF, 2);
	if (r->keepalive)
	{
		http_append_to_output_buf(r, "Connection: keep-alive", 22);
		http_append_to_output_buf(r, CLRF, 2);
		http_append_to_output_buf(r, "Keep-Alive: timeout=", 20);
		http_append_to_output_buf(r, config.keepalive_timeout.str, config.keepalive_timeout.len);
	}
	else
		http_append_to_output_buf(r, "Connection: close", 17);
	http_append_to_output_buf(r, CLRF, 2);
}

static inline bool http_send (request_t * r)
{
	ssize_t res;
	uint size, i, d;
	struct iovec * data;
	
	data = (struct iovec *) r->out_vec->data;
	
	res = writev(r->sock, data, r->out_vec->cur_len);
	
	if (res == -1 && errno != EAGAIN)
	{
		perr("writev(): %d", -1);
		
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

static bool http_error (request_t * r, ushort code)
{
	r->out.content_length = http_error_message_len[code];
	r->out.content_type.str = (uchar *) HTTP_ERROR_CONTENT_TYPE;
	r->out.content_type.len = HTTP_ERROR_CONTENT_TYPE_LEN;
	http_append_headers(r, code);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, (void *) http_error_message[code], r->out.content_length);
	
	return http_send(r);
}

static inline uchar http_hex_to_ascii (uchar * c)
{
	uchar l, r;
	
	l = TO_LOWER(c[1]);
	r = ((l >= '0' && l <= '9') ? l - '0' : l - 'a' + 10) * 16;
	l = TO_LOWER(c[2]);
	r += (l >= '0' && l <= '9') ? l - '0' : l - 'a' + 10;
	
	return r;
}

static bool http_divide_uri (request_t * r)
{
	uchar * c, * d;
	uint i;
	
	if (r->in.uri.len > HTTP_PATH_PREALLOC)
		r->in.path.str = (uchar *) realloc(r->in.path.str, r->in.uri.len);
	c = r->in.path.str;
	d = r->in.uri.str;
	
	* (c++) = '/';
	
	for (i = 1; i < r->in.uri.len && d[i] != '?'; i++, c++)
	{
		if (d[i] == '%' && i < r->in.uri.len - 2)
		{
			* c = http_hex_to_ascii(d + i);
			i += 2;
		}
		else if (IS_VALID_PATH_CHARACTER(d[i]))
			* c = d[i];
		else
			return false;
	}
	
	* c = '\0';
	r->in.path.len = c - r->in.path.str;
	
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

static inline bool http_send_file (request_t * r, const char * filepath)
{
	uint i, it;
	bool ret;
	struct stat st;
	uchar * fileext;
	int fd, t;
	
	fd = open(filepath, O_RDONLY);
	
	if (fd == -1)
	{
		if (errno == ENOENT || errno == ENOTDIR)
			return http_error(r, 404);
		
		if (errno == EACCES)
			return http_error(r, 403);
		
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
	
	ushort code = 200;
	ssize_t res;
	header_t * hdr;
	struct tm c_time, m_time;
	time_t curtime;
	
	char * rfc822_date_str = r->temp.dates;
	
	r->temp.sendfile_last = st.st_size;
	r->temp.sendfile_offset = 0;
	
	gmtime_r(&(st.st_mtime), &m_time);
	http_rfc822_date(rfc822_date_str, &m_time);
	
	for (i = 0; i < r->in.p->cur_len; i++)
	{
		hdr = (header_t *) r->in.p->data[i];
		if (hdr->key.len == 17 && hdr->value.len == 29 && smemcmp(hdr->key.str, (uchar *) "if-modified-since", 17) && smemcmp(hdr->value.str, (uchar *) rfc822_date_str, 29))
		{
			code = 304;
			break;
		}
		else if (hdr->key.len == 5 && smemcmp(hdr->key.str, (uchar *) "range", 5) && hdr->value.len > 6 && smemcmp(hdr->value.str, (uchar *) "bytes=", 6))
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
	
	curtime = time(NULL);
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
	
	res = sendfile(r->sock, fd, &(r->temp.sendfile_offset), (size_t) r->out.content_length);
	
	if (res == -1 && errno != EAGAIN)
	{
		perr("sendfile(): %d", (int) res);
		
		return true;
	}
	if (res < r->out.content_length)
	{
		r->temp.sendfile_fd = fd;
		set_epollout_event_mask(r->sock);
		
		return false;
	}
	
	close(fd);
	
	return true;
}

static bool http_response (request_t * r)
{
	uint i;
	int res;
	buf_t * buf;
	bool accept_gzip = false;
	struct tm c_time;
	time_t curtime;
	uri_map_t * m = (uri_map_t *) uri_map->data;
	
	if (!http_divide_uri(r))
		return http_error(r, 400);
	
	for (i = 0; i < uri_map->cur_len; i++)
		if (strcmp((char *) r->in.uri.str, m[i].uri) == 0)
		{
			r->out.content_type.str = (uchar *) "text/html";
			r->out.content_type.len = 9;
			
			buf = m[i].func(r);
			
			if (config.gzip && buf->cur_len > config.gzip_min_page_size)
			{
				header_t * hdr;
				
				for (i = 0; i < r->in.p->cur_len; i++)
				{
					hdr = (header_t *) r->in.p->data[i];
					if (hdr->key.len == 15 && smemcmp(hdr->key.str, (uchar *) "accept-encoding", 15))
					{
						if (strstr((const char *) hdr->value.str, "gzip") != NULL)
							accept_gzip = true;
					}
				}
			}
			
			if (accept_gzip)
			{
				z_stream * z = r->temp.gzip_stream;
				buf_expand(r->temp.gzip_buf, buf->cur_len);
				z->avail_in = buf->cur_len;
				z->next_in  = buf->data;
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
						
						return http_error(r, 500);
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
			
			curtime = time(NULL);
			gmtime_r(&curtime, &c_time);
			http_rfc822_date(r->temp.dates, &c_time);
			http_append_to_output_buf(r, "Date: ", 6);
			http_append_to_output_buf(r, r->temp.dates, 29);
			http_append_to_output_buf(r, CLRF CLRF, 4);
			
			if (accept_gzip)
			{
				http_append_to_output_buf(r, (void *) gzip_header, 10);
				http_append_to_output_buf(r, r->temp.gzip_buf->data, i);
				r->temp.gzip_ending[0] = crc32(0, buf->data, buf->cur_len);
				r->temp.gzip_ending[1] = buf->cur_len;
				http_append_to_output_buf(r, (void *) r->temp.gzip_ending, 8);
			}
			else
				http_append_to_output_buf(r, buf->data, r->out.content_length);
			
			return http_send(r);
		}
	
	buf_expand(r->temp.filepath, config.document_root.len + r->in.path.len + 1);
	memcpy((char *) r->temp.filepath->data + config.document_root.len, r->in.path.str, r->in.path.len + 1);
	
	return http_send_file(r, (const char *) r->temp.filepath->data);
}

static ushort http_parse_headers (request_t * r)
{
	uchar * p = (uchar *) r->b->data;
	
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
	
	header_t * header;
	uchar * key, * val;
	
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
		header->key.len == 10 && strcmp((const char *) header->key.str, "connection") == 0 &&
		/* For performance reasons we compare only first and last letter of connection header value (looking for "[k]eep-aliv[e]"), not full string */
		header->value.len > 9 && TO_LOWER(header->value.str[0]) == 'k' && TO_LOWER(header->value.str[9]) == 'e'
		)
			r->keepalive = true;
		
		debug_print_3("header \"%s: %s\"", header->key.str, header->value.str);
		
		p += 2;
	}
	
	return 0;
}

bool http_serve_client (request_t * request)
{
	ushort code;
	int r, _r;
	uint i;
	header_t * hdr;
	uchar * buf;
	
	buf_expand(request->b, HTTP_RECV_BUFFER);
	buf = (uchar *) request->b->data + request->b->cur_len - HTTP_RECV_BUFFER;
	
	while ((r = recv(request->sock, buf, HTTP_RECV_BUFFER, MSG_DONTWAIT)) > 0)
	{
		_r = r;
		
		request->b->cur_len -= HTTP_RECV_BUFFER - r;
		
		if (request->in.uri.len == 0)
		{
			if (request->b->cur_len > HTTP_MAX_HEADERS_SIZE)
				return http_error(request, 414);
			
			for (i = 4; i < r; i++)
			{
				if (buf[i] == '\n' && buf[i - 1] == '\r' && buf[i - 2] == '\n' && buf[i - 3] == '\r')
				{
					buf[i - 1] = 1;
					code = http_parse_headers(request);
					
					if (code)
						return http_error(request, code);
					
					if (!(request->in.method_post))
						return http_response(request);
					else
					{
						buf += i + 1;
						r -= i + 1;
						request->in.body.data = buf;
						request->in.body.data_len = 0;
						
						for (i = 0; i < request->in.p->cur_len; i++)
						{
							hdr = (header_t *) request->in.p->data[i];
							if (hdr->key.len == 12 && request->in.content_type == NULL && smemcmp(hdr->key.str, (uchar *) "content-type", 12))
								request->in.content_type = hdr;
							else if (hdr->key.len == 14 && request->in.content_length == NULL && smemcmp(hdr->key.str, (uchar *) "content-length", 14))
								request->in.content_length = hdr;
						}
						
						if (request->in.content_type == NULL)
							return http_error(request, 400);
						
						if (request->in.content_length == NULL)
							return http_error(request, 411);
						
						request->in.multipart = (request->in.urlenc = false);
						if (request->in.content_type->value.len == 33 && smemcmp(request->in.content_type->value.str, (uchar *) "application/x-www-form-urlencoded", 33))
							request->in.urlenc = true;
						else if (request->in.content_type->value.len > 19 && smemcmp(request->in.content_type->value.str, (uchar *) "multipart/form-data", 19))
							request->in.multipart = true;
						else
							return http_error(request, 415);
						
						request->in.content_length_val = (uint) atoi((const char *) request->in.content_length->value.str);
						if (request->in.content_length_val > HTTP_MAX_REQUEST_BODY_SIZE)
							return http_error(request, 413);
						
						goto _post_parse;
					}
				}
			}
			
			goto _loop_end;
		}
		
		_post_parse:;
		
		if (request->in.content_length_val < HTTP_BODY_SIZE_WRITE_TO_FILE)
		{
			request->in.body.data_len += r;
			
			if (request->in.body.data_len < request->in.content_length_val)
				goto _loop_end;
		}
		else if (request->temp.tempfd == -1)
		{
			int_to_str(request->sock, request->temp.temppath + request->temp.temppath_len, 16);
			request->temp.tempfd = open(request->temp.temppath, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
			if (request->temp.tempfd == -1)
			{
				perr("open(): %d", -1);
				
				return true;
			}
			if (pipe(request->temp.pipefd) == -1)
			{
				perr("pipe(): %d", -1);
				
				return true;
			}
			if (write(request->temp.tempfd, buf, r) == -1)
			{
				perr("write(): %d", -1);
				
				return true;
			}
			
			request->temp.pos = r;
			
			return false;
		}
		
		return http_response(request);
		
		_loop_end:;
		
		if (_r < HTTP_RECV_BUFFER)
			return false;
		
		buf_expand(request->b, HTTP_RECV_BUFFER);
		buf = (uchar *) request->b->data + request->b->cur_len - HTTP_RECV_BUFFER;
	}
	
	if (r == -1 && errno == EAGAIN)
		return false;
	
	#if DEBUG_LEVEL
	if (r == -1)
		perr("recv(): %d", r);
	#endif
	
	request->keepalive = false;
	
	return true;
}

bool http_temp_file (request_t * r)
{
	int ret, count;
	
	for (count = 0;;)
	{
		ret = splice(r->sock, NULL, r->temp.pipefd[1], NULL, r->in.content_length_val, SPLICE_F_NONBLOCK);
		
		if ((ret == -1 && errno == EAGAIN) || ret == 0)
			break;
		
		if (ret == -1)
		{
			perr("splice(): %d", -1);
			
			return true;
		}
		
		r->temp.pos += ret;
		count += ret;
	}
	
	if (count == 0)
		return true;
	
	ret = splice(r->temp.pipefd[0], NULL, r->temp.tempfd, NULL, count, 0);
	
	if (ret == -1)
	{
		perr("splice(): %d", -1);
		
		return true;
	}
	
	
	if (r->temp.pos < r->in.content_length_val)
		return false;
	
	close(r->temp.pipefd[0]);
	close(r->temp.pipefd[1]);
	close(r->temp.tempfd);
	
	return http_response(r);
}

void http_cleanup (request_t * r)
{
	if (r->keepalive)
	{
		set_epollin_event_mask(r->sock);
		r->keepalive_time = time(NULL);
	}
	else
	{
		r->keepalive = false;
		close(r->sock);
		debug_print_3("close(): %d", r->sock);
		r->sock = -1;
	}
	
	r->in.uri.len = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	
	r->temp.writev_total = 0;
	r->temp.sendfile_fd = -1;
	
	if (config.gzip)
		buf_free(r->temp.gzip_buf);
	
	buf_free(r->temp.filepath);
	buf_free(r->out_vec);
	buf_free(r->b);
	
	pool_free(r->in.p, HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
	
	if (r->in.method_post)
	{
		r->temp.tempfd = -1;
	}
}

static void http_init_constants (void);

void http_prepare (request_t * r)
{
	int res;
	
	http_init_constants();
	
	header_server_string.str = "Server: " SERVER_STRING;
	header_server_string.len = strlen(header_server_string.str);
	
	r->temp.temppath = (char *) malloc(512);
	r->temp.temppath_len = strlen(config.temp_dir);
	memcpy(r->temp.temppath, config.temp_dir, r->temp.temppath_len);
	if (r->temp.temppath[r->temp.temppath_len - 1] != '/')
		r->temp.temppath[r->temp.temppath_len++] = '/';
	r->temp.temppath[r->temp.temppath_len] = '\0';
	
	r->sock = -1;
	r->keepalive = false;
	r->keepalive_time = 1;
	r->in.method_get = (r->in.method_post = (r->in.method_head = false));
	r->in.uri.len = 0;
	r->in.path.str = (uchar *) malloc(HTTP_PATH_PREALLOC);
	r->in.path.len = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	r->temp.tempfd = -1;
	r->temp.pos = 0;
	r->temp.writev_total = 0;
	r->temp.sendfile_fd = -1;
	
	r->out.content_range.str = (char *) malloc(64);
	r->out.content_range.len = 0;
	r->out.expires.str = (char *) malloc(29);
	r->out.expires.len = 29;
	
	if (config.gzip)
	{
		r->temp.gzip_buf = buf_create(1, HTTP_GZIP_BUFFER_RESERVED_SIZE);
		r->temp.gzip_stream = (z_stream *) malloc(sizeof(z_stream));
		memset(r->temp.gzip_stream, 0, sizeof(z_stream));
		res = deflateInit2(r->temp.gzip_stream, config.gzip_level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		if (res != Z_OK)
			peerr(1, "deflateInit2(): %d", res);
	}
	
	r->temp.filepath = buf_create(1, config.document_root.len + HTTP_PATH_PREALLOC);
	memcpy(r->temp.filepath->data, config.document_root.str, config.document_root.len);
	
	r->out_vec = buf_create(sizeof(struct iovec), HTTP_OUTPUT_VECTOR_START_SIZE);
	
	r->b = buf_create(1, HTTP_BUFFER_RESERVED_SIZE);
	r->in.p = pool_create(sizeof(header_t), HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
}

static void http_init_constants (void)
{
	/* 2xx */
	http_status_code[200] = "200 OK";
	http_status_code_len[200] = strlen(http_status_code[200]);
	http_status_code[206] = "206 Partial Content";
	http_status_code_len[206] = strlen(http_status_code[206]);
	/* 3xx */
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
