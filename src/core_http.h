void http_append_to_output_buf (request_t * r, void * pointer, uint len);

bool http_serve_client (request_t * request);

bool http_temp_file (request_t * r);

void http_cleanup (request_t * r);

void http_prepare (request_t * r);
