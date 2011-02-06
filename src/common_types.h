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
	uint cur_len;
	uint reserved_len;
	uint node_size;
	void * data;
} buf_t;

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
	buf_t * b;
} content_body_t;

typedef struct
{
	uchar buf[HTTP_MAX_HEADERS_SIZE];
	uint buf_pos;
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
	str_t status;
	u_str_t http_version;
	ulong long content_length;
	u_str_t content_type;
} headers_out_t;

typedef struct
{
	pool_t * p;
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
		uint pos;
		buf_t * filepath;
		int sendfile_fd;
		size_t sendfile_count;
		size_t sendfile_total;
	} temp;
} request_t;

typedef struct
{
	const char * uri;
	void * func;
} uri_map_t;

typedef struct
{
	const char * user;
	const char * group;
	const char * temp_dir;
	u_str_t document_root;
} config_t;
