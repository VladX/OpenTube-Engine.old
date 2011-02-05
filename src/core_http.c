#define _GNU_SOURCE
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h> 
#include "common_functions.h"

#define HTTP_HTML_PAGE_TEMPLATE_TOP \
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n" \
	"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n" \
	"<head>\n" \
	"	<title>"

#define HTTP_HTML_PAGE_TEMPLATE_CON \
	"</title>\n" \
	"</head>\n\n" \
	"<body>\n" \
	"	<div align=\"center\">\n" \
	"		<h1>"

#define HTTP_HTML_PAGE_TEMPLATE_BOT \
	"</h1>\n" \
	"	</div>\n" \
	"	<hr />\n" \
	"	" SERVER_STRING "\n" \
	"</body>\n" \
	"</html>\n"

#define HTTP_ERROR_CONTENT_TYPE "text/html"
#define HTTP_ERROR_CONTENT_TYPE_LEN 9

#define HTTP_OUTPUT_VECTOR_START_SIZE 64


static const char * http_error_message[506];
static const char * http_status_code[506];
static ushort http_error_message_len[506];
static uchar http_status_code_len[506];
static uchar server_string_len;


static inline void http_append_to_output_buf (request_t * r, void * pointer, uint len)
{
	struct iovec * iov;
	uint offset;
	
	offset = r->out_vec->cur_len;
	
	buf_expand(r->out_vec, 1);
	
	iov = (struct iovec *) r->out_vec->data;
	
	iov[offset].iov_base = pointer;
	iov[offset].iov_len = len;
}

static inline void http_append_str (request_t * r, char * str)
{
	http_append_to_output_buf(r, str, strlen((const char *) str));
}

static inline void http_append_headers (request_t * r)
{
	char temp[16];
	
	http_append_to_output_buf(r, "HTTP/1.1 ", 9);
	http_append_to_output_buf(r, r->out.status.str, r->out.status.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Server: " SERVER_STRING, server_string_len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Type: ", 14);
	http_append_to_output_buf(r, r->out.content_type.str, r->out.content_type.len);
	http_append_to_output_buf(r, CLRF, 2);
	http_append_to_output_buf(r, "Content-Length: ", 16);
	int_to_str(r->out.content_length, temp, 10);
	http_append_str(r, temp);
	http_append_to_output_buf(r, CLRF CLRF, 4);
}

static inline void http_send (request_t * r)
{
	if (writev(r->sock, (struct iovec *) r->out_vec->data, r->out_vec->cur_len) == -1)
		perr("writev(): %d", -1);
}

static void http_error (request_t * r, ushort code)
{
	r->out.content_length = http_error_message_len[code];
	r->out.status.str = (char *) http_status_code[code];
	r->out.status.len = http_status_code_len[code];
	r->out.content_type.str = (uchar *) HTTP_ERROR_CONTENT_TYPE;
	r->out.content_type.len = HTTP_ERROR_CONTENT_TYPE_LEN;
	http_append_headers(r);
	http_append_to_output_buf(r, (void *) http_error_message[code], r->out.content_length);
	http_send(r);
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
		{
			http_error(r, 400);
			
			return false;
		}
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
	
	debug_print_3("%s %d", r->in.query_string.str, r->in.query_string.len);
	
	return true;
}

static void http_response (request_t * r)
{
	uint i;
	uri_map_t * m = (uri_map_t *) uri_map->data;
	
	if (!http_divide_uri(r))
		return;
	
	/*for (i = 0; i < uri_map->cur_len; i++)
		if (strcmp(r->in.uri.str, m[i].uri) == 0)
		{
			m[i].func();
			return;
		}
	
	struct stat file_stat;
	int fd;
	
	fd = open("", O_RDONLY);*/
	
	http_error(r, 404);
}

static ushort http_parse_headers (request_t * r)
{
	uchar * p = r->in.buf;
	
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
		
		debug_print_3("header \"%s: %s\"", header->key.str, header->value.str);
		
		p += 2;
	}
	
	return 0;
}

bool http_serve_client (request_t * request)
{
	ushort code;
	int r, _r;
	uint i, n;
	header_t * hdr;
	uchar * buf, * last;
	
	buf = pool_alloc(request->p);
	
	while ((r = recv(request->sock, buf, HTTP_POOL_FRAGMENT_SIZE, MSG_DONTWAIT)) > 0)
	{
		_r = r;
		
		if (request->in.uri.len == 0)
		{
			if (request->in.buf_pos + r > HTTP_MAX_HEADERS_SIZE)
			{
				http_error(request, 414);
				return true;
			}
			memcpy(request->in.buf + request->in.buf_pos, buf, r);
			
			for (i = 4; i < r; i++)
			{
				if (buf[i] == '\n' && buf[i - 1] == '\r' && buf[i - 2] == '\n' && buf[i - 3] == '\r')
				{
					request->in.buf[request->in.buf_pos + i - 1] = 1;
					code = http_parse_headers(request);
					if (code)
					{
						http_error(request, code);
						return true;
					}
					if (!(request->in.method_post))
					{
						http_response(request);
						
						return true;
					}
					else
					{
						last = buf + r;
						buf += i + 1;
						r = last - buf;
						for (i = 0; i < request->in.p->cur_len; i++)
						{
							hdr = (header_t *) request->in.p->data[i];
							if (hdr->key.len == 12 && request->in.content_type == NULL && smemcmp(hdr->key.str, (uchar *) "content-type", 12))
								request->in.content_type = hdr;
							else if (hdr->key.len == 14 && request->in.content_length == NULL && smemcmp(hdr->key.str, (uchar *) "content-length", 14))
								request->in.content_length = hdr;
						}
						if (request->in.content_type == NULL)
						{
							http_error(request, 400);
							
							return true;
						}
						if (request->in.content_length == NULL)
						{
							http_error(request, 411);
							
							return true;
						}
						request->in.multipart = (request->in.urlenc = false);
						if (request->in.content_type->value.len == 33 && smemcmp(request->in.content_type->value.str, (uchar *) "application/x-www-form-urlencoded", 33))
							request->in.urlenc = true;
						else if (request->in.content_type->value.len > 19 && smemcmp(request->in.content_type->value.str, (uchar *) "multipart/form-data", 19))
							request->in.multipart = true;
						else
						{
							http_error(request, 415);
							
							return true;
						}
						request->in.content_length_val = (uint) atoi((const char *) request->in.content_length->value.str);
						if (request->in.content_length_val > HTTP_MAX_REQUEST_BODY_SIZE)
						{
							http_error(request, 413);
							
							return true;
						}
						goto _post_parse;
					}
				}
			}
			request->in.buf_pos += r;
			goto _loop_end;
		}
		
		last = buf + r;
		
		_post_parse:;
		
		if (request->in.content_length_val < HTTP_BODY_SIZE_WRITE_TO_FILE)
		{
			if (request->in.body.b->cur_len < request->in.content_length_val)
			{
				n = request->in.body.b->cur_len;
				buf_expand(request->in.body.b, r);
				memcpy((uchar *) request->in.body.b->data + n, buf, r);
				if (request->in.body.b->cur_len < request->in.content_length_val)
					goto _loop_end;
			}
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
		
		http_response(request);
		
		return true;
		
		_loop_end:;
		
		if (_r < HTTP_POOL_FRAGMENT_SIZE)
			return false;
		
		buf = pool_alloc(request->p);
	}
	
	if (r < 0 && errno == EAGAIN)
		return false;
	#if DEBUG_LEVEL
	else if (r < 0)
		perr("recv(): %d", r);
	#endif
	
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
	
	http_response(r);
	
	return true;
}

void http_cleanup (request_t * r)
{
	r->sock = -1;
	
	r->in.uri.len = 0;
	r->in.path.str = NULL;
	r->in.buf_pos = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	
	buf_free(r->out_vec);
	
	pool_free(r->p, HTTP_POOL_RESERVED_FRAGMENTS);
	pool_free(r->in.p, HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
	
	if (r->in.method_post)
	{
		r->temp.tempfd = -1;
		buf_free(r->in.body.b);
	}
}

void http_init_constants (void)
{
	/* 4xx */
	http_error_message[400] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "400 - Bad Request"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Bad Request"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[400] = strlen(http_error_message[400]);
	http_status_code[400] = "400 Bad Request";
	http_status_code_len[400] = strlen(http_status_code[400]);
	http_error_message[401] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "401 - Unauthorized"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Unauthorized"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[401] = strlen(http_error_message[401]);
	http_status_code[401] = "401 Unauthorized";
	http_status_code_len[401] = strlen(http_status_code[401]);
	http_error_message[402] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "402 - Payment Required"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Payment Required"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[402] = strlen(http_error_message[402]);
	http_status_code[402] = "402 Payment Required";
	http_status_code_len[402] = strlen(http_status_code[402]);
	http_error_message[403] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "403 - Forbidden"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Forbidden"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[403] = strlen(http_error_message[403]);
	http_status_code[403] = "403 Forbidden";
	http_status_code_len[403] = strlen(http_status_code[403]);
	http_error_message[404] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "404 - Not Found"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Not Found"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[404] = strlen(http_error_message[404]);
	http_status_code[404] = "404 Not Found";
	http_status_code_len[404] = strlen(http_status_code[404]);
	http_error_message[405] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "405 - Method Not Allowed"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Method Not Allowed"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[405] = strlen(http_error_message[405]);
	http_status_code[405] = "405 Method Not Allowed";
	http_status_code_len[405] = strlen(http_status_code[405]);
	http_error_message[406] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "406 - Not Acceptable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Not Acceptable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[406] = strlen(http_error_message[406]);
	http_status_code[406] = "406 Not Acceptable";
	http_status_code_len[406] = strlen(http_status_code[406]);
	http_error_message[408] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "408 - Request Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Request Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[408] = strlen(http_error_message[408]);
	http_status_code[408] = "408 Request Timeout";
	http_status_code_len[408] = strlen(http_status_code[408]);
	http_error_message[411] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "411 - Length Required"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Length Required"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[411] = strlen(http_error_message[411]);
	http_status_code[411] = "411 Length Required";
	http_status_code_len[411] = strlen(http_status_code[411]);
	http_error_message[413] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "413 - Request Entity Too Large"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Request Entity Too Large"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[413] = strlen(http_error_message[413]);
	http_status_code[413] = "413 Request Entity Too Large";
	http_status_code_len[413] = strlen(http_status_code[413]);
	http_error_message[414] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "414 - Request-URL Too Long"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Request-URL Too Long"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[414] = strlen(http_error_message[414]);
	http_status_code[414] = "414 Request-URL Too Long";
	http_status_code_len[414] = strlen(http_status_code[414]);
	http_error_message[415] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "415 - Unsupported Media Type"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Unsupported Media Type"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[415] = strlen(http_error_message[415]);
	http_status_code[415] = "415 Unsupported Media Type";
	http_status_code_len[415] = strlen(http_status_code[415]);
	http_error_message[416] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "416 - Requested Range Not Satisfiable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Requested Range Not Satisfiable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[416] = strlen(http_error_message[416]);
	http_status_code[416] = "416 Requested Range Not Satisfiable";
	http_status_code_len[416] = strlen(http_status_code[416]);
	/* 5xx */
	http_error_message[500] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "500 - Internal Server Error"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Internal Server Error"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[500] = strlen(http_error_message[500]);
	http_status_code[500] = "500 Internal Server Error";
	http_status_code_len[500] = strlen(http_status_code[500]);
	http_error_message[501] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "501 - Not Implemented"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Not Implemented"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[501] = strlen(http_error_message[501]);
	http_status_code[501] = "501 Not Implemented";
	http_status_code_len[501] = strlen(http_status_code[501]);
	http_error_message[502] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "502 - Bad Gateway"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Bad Gateway"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[502] = strlen(http_error_message[502]);
	http_status_code[502] = "502 Bad Gateway";
	http_status_code_len[502] = strlen(http_status_code[502]);
	http_error_message[503] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "503 - Service Unavailable"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Service Unavailable"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[503] = strlen(http_error_message[503]);
	http_status_code[503] = "503 Service Unavailable";
	http_status_code_len[503] = strlen(http_status_code[503]);
	http_error_message[504] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "504 - Gateway Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "Gateway Timeout"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[504] = strlen(http_error_message[504]);
	http_status_code[504] = "504 Gateway Timeout";
	http_status_code_len[504] = strlen(http_status_code[504]);
	http_error_message[505] = HTTP_HTML_PAGE_TEMPLATE_TOP
	                          "505 - HTTP Version Not Supported"
	                          HTTP_HTML_PAGE_TEMPLATE_CON
	                          "HTTP Version Not Supported"
	                          HTTP_HTML_PAGE_TEMPLATE_BOT
	;
	http_error_message_len[505] = strlen(http_error_message[505]);
	http_status_code[505] = "505 HTTP Version Not Supported";
	http_status_code_len[505] = strlen(http_status_code[505]);
}

void http_prepare (request_t * r)
{
	http_init_constants();
	
	server_string_len = strlen(SERVER_STRING) + 8;
	
	r->temp.temppath = (char *) malloc(512);
	r->temp.temppath_len = strlen(config.temp_dir);
	memcpy(r->temp.temppath, config.temp_dir, r->temp.temppath_len);
	if (r->temp.temppath[r->temp.temppath_len - 1] != '/')
		r->temp.temppath[r->temp.temppath_len++] = '/';
	r->temp.temppath[r->temp.temppath_len] = '\0';
	
	r->sock = -1;
	r->in.method_get = (r->in.method_post = (r->in.method_head = false));
	r->in.uri.len = 0;
	r->in.path.str = NULL;
	r->in.buf_pos = 0;
	r->in.content_type = NULL;
	r->in.content_length = NULL;
	r->temp.tempfd = -1;
	r->temp.pos = 0;
	
	r->out_vec = buf_create(sizeof(struct iovec), HTTP_OUTPUT_VECTOR_START_SIZE);
	
	r->p = pool_create(HTTP_POOL_FRAGMENT_SIZE, HTTP_POOL_RESERVED_FRAGMENTS);
	r->in.p = pool_create(sizeof(header_t), HTTP_HEADERS_POOL_RESERVED_FRAGMENTS);
	r->in.body.b = buf_create(1, HTTP_POST_BUFFER_RESERVED_SIZE);
}
