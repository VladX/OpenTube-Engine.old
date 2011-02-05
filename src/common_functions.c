#include "common_functions.h"


void set_str (str_t * str, char * src)
{
	str->len = strlen(src);
	str->str = src;
}

void set_cpy_str (str_t * str, char * src)
{
	str->len = strlen(src);
	str->str = (char *) malloc(str->len + 1);
	memcpy(str->str, src, str->len + 1);
}

str_t * new_str (char * src)
{
	str_t * str = (str_t *) malloc(sizeof(str_t));
	set_cpy_str(str, src);
	
	return str;
}

pool_t * pool_create (uint size, uint start_len)
{
	pool_t * p;
	uint i;
	
	p = (pool_t *) malloc(sizeof(pool_t));
	p->cur_len = 0;
	p->real_len = start_len;
	p->node_size = size;
	p->data = malloc(sizeof(void *) * p->real_len);
	
	for (i = 0; i < p->real_len; i++)
		p->data[i] = malloc(p->node_size);
	
	return p;
}

void * pool_alloc (pool_t * p)
{
	if (p->cur_len >= p->real_len)
	{
		p->data = realloc(p->data, sizeof(void *) * (p->cur_len + 1));
		p->data[p->cur_len] = malloc(p->node_size);
		p->real_len++;
	}
	
	return p->data[p->cur_len++];
}

void pool_free (pool_t * p, uint len)
{
	uint i = len;
	
	for (; i < p->real_len; i++)
		free(p->data[i]);
	
	p->cur_len = 0;
	p->real_len = len;
}

void pool_free_last (pool_t * p, uint len)
{
	p->cur_len--;
	if (p->cur_len >= len)
	{
		p->real_len--;
		
		free(p->data[p->cur_len]);
	}
}

buf_t * buf_create (ulong size, ulong res_len)
{
	buf_t * b;
	
	b = (buf_t *) malloc(sizeof(buf_t));
	b->cur_len = 0;
	b->reserved_len = res_len;
	b->node_size = size;
	b->data = malloc(size * res_len);
	
	return b;
}

void buf_destroy (buf_t * b)
{
	free(b->data);
	free(b);
}

void buf_expand (buf_t * b, uint add)
{
	b->cur_len += add;
	
	if (b->cur_len > b->reserved_len)
		b->data = realloc(b->data, b->node_size * b->cur_len);
}

void buf_free (buf_t * b)
{
	if (b->cur_len > b->reserved_len)
		b->data = realloc(b->data, b->node_size * b->reserved_len);
	
	b->cur_len = 0;
}

void int_to_str (int value, char * result, int base)
{
	char * ptr = result, * ptr1 = result, tmp_char;
	int tmp_value;
	
	do
	{
		tmp_value = value;
		value /= base;
		* (ptr++) = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while (value);
	
	if (tmp_value < 0)
		* (ptr++) = '-';
	
	* (ptr--) = '\0';
	
	while (ptr1 < ptr)
	{
			tmp_char = * ptr;
			* (ptr--) = * ptr1;
			* (ptr1++) = tmp_char;
	}
}

bool smemcmp (uchar * p1, uchar * p2, uint len)
{
	uint i;
	
	for (i = 0; i < len; i++)
		if (p1[i] != p2[i])
			return false;
	
	return true;
}
