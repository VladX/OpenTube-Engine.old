#define PROG_NAME "Opentube"
#define PROG_VERSION "0.1alpha"
#define DEBUG_LEVEL 3 // 0 - disabled
#define SERVER_STRING PROG_NAME "/" PROG_VERSION
#define IPV6_SUPPORT 1
#define WORKER_THREADS 10
#define MAX_EVENTS 10
#define CONFIG_PATH "./server.conf"
#define LISTEN_BACKLOG 500
#define HTTP_POOL_FRAGMENT_SIZE 4096
#define HTTP_POOL_RESERVED_FRAGMENTS 16
#define HTTP_HEADERS_POOL_RESERVED_FRAGMENTS 32
#define HTTP_POST_POOL_RESERVED_FRAGMENTS 32
#define HTTP_POST_BUFFER_RESERVED_SIZE 32768
#define HTTP_BODY_SIZE_WRITE_TO_FILE 8388608 // 8 Mb
#define HTTP_MAX_REQUEST_BODY_SIZE 33554432 // 32 Mb
#define HTTP_MAX_HEADERS_SIZE HTTP_POOL_FRAGMENT_SIZE
#define HTTP_STATIC_EXPIRES 172800 // 2 days

#define HAVE_ACCEPT4 1
