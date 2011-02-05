#include "common_functions.h"


void map_uri_to_function (const char * uri, void * func)
{
	buf_expand(uri_map, 1);
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].uri = uri;
	((uri_map_t *) uri_map->data)[uri_map->cur_len - 1].func = func;
}
