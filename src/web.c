#include "common_functions.h"
#include "core_http.h"
#include "core_web.h"


buf_t * test_func (request_t * r)
{
	const char * hello_world = "Hello World!";
	
	buf_t * b = buf_create(1, 0);
	int l = strlen(hello_world);
	
	b->data = (void *) hello_world;
	b->cur_len = l;
	
	return b;
}

void web_init (void)
{
	map_uri_to_function("/hello", test_func);
}
