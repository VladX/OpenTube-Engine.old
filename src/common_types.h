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
	u_str_t key;
	u_str_t value;
} header_t;

typedef struct
{
	u_str_t key;
	u_str_t value;
} post_arg_t;

typedef struct
{
	post_arg_t * args;
	uchar * data;
	uint data_len;
} content_body_t;

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
	content_body_t body;
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
	buf_t * out_vec;
	int sock;
	struct
	{
		char * temppath;
		int temppath_len;
		int tempfd;
		int pipefd[2];
		long writev_total;
		struct iovec * out_vec;
		uint out_vec_len;
		uint pos;
		buf_t * filepath;
		int sendfile_fd;
		uint sendfile_last;
		off_t sendfile_offset;
		char dates[60];
		buf_t * gzip_buf;
		z_stream * gzip_stream;
		__uint32_t gzip_ending[2];
	} temp;
} request_t;

typedef buf_t * (* web_func_t) (request_t *);

typedef struct
{
	const char * uri;
	web_func_t func;
} uri_map_t;

typedef struct
{
	const char * user;
	const char * group;
	const char * temp_dir;
	u_str_t document_root;
	bool gzip;
	uchar gzip_level;
	uint gzip_min_page_size;
	bool limit_req;
	uint limit_rate;
	uint limit_delay;
} config_t;
