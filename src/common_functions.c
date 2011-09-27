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

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "os_stat.h"
#include "common_functions.h"


void set_str (str_t * str, char * src)
{
	str->len = strlen(src);
	str->str = src;
}

void set_cpy_str (str_t * str, char * src)
{
	str->len = strlen(src);
	str->str = (char *) allocator_malloc(str->len + 1);
	memcpy(str->str, src, str->len + 1);
}

str_t * new_str (char * src)
{
	str_t * str = (str_t *) allocator_malloc(sizeof(str_t));
	set_cpy_str(str, src);
	
	return str;
}

void set_ustr (u_str_t * str, uchar * src)
{
	str->len = strlen((char *) src);
	str->str = src;
}

void set_cpy_ustr (u_str_t * str, uchar * src)
{
	str->len = strlen((char *) src);
	str->str = (uchar *) allocator_malloc(str->len + 1);
	memcpy(str->str, src, str->len + 1);
}

u_str_t * new_ustr (uchar * src)
{
	u_str_t * str = (u_str_t *) allocator_malloc(sizeof(u_str_t));
	set_cpy_ustr(str, src);
	
	return str;
}

pool_t * pool_create (uint size, uint start_len)
{
	register pool_t * p;
	register uint i;
	
	p = (pool_t *) allocator_malloc(sizeof(pool_t));
	p->cur_len = 0;
	p->real_len = start_len;
	p->node_size = size;
	p->data = allocator_malloc(sizeof(void *) * p->real_len);
	
	for (i = 0; i < p->real_len; i++)
		p->data[i] = allocator_malloc(p->node_size);
	
	return p;
}

void * pool_alloc (pool_t * p)
{
	if (p->cur_len >= p->real_len)
	{
		p->data = allocator_realloc(p->data, sizeof(void *) * (p->cur_len + 1));
		p->data[p->cur_len] = allocator_malloc(p->node_size);
		p->real_len++;
	}
	
	return p->data[p->cur_len++];
}

void pool_free (pool_t * p, uint len)
{
	register uint i;
	
	for (i = len; i < p->real_len; i++)
		allocator_free(p->data[i]);
	
	p->cur_len = 0;
	p->real_len = len;
}

void pool_free_last (pool_t * p, uint len)
{
	p->cur_len--;
	if (p->cur_len >= len)
	{
		p->real_len--;
		
		allocator_free(p->data[p->cur_len]);
	}
}

void pool_destroy (pool_t * p)
{
	register uint i;
	
	for (i = 0; i < p->real_len; i++)
		allocator_free(p->data[i]);
	
	allocator_free(p->data);
	allocator_free(p);
}

frag_pool_t * frag_pool_create (uint size, uint res_len)
{
	register frag_pool_t * p;
	register uint i;
	
	p = (frag_pool_t *) allocator_malloc(sizeof(frag_pool_t));
	p->cur_len = 0;
	p->reserved_len = res_len;
	p->real_len = res_len;
	p->node_size = size;
	p->e = (frag_pool_elem_t *) allocator_malloc(sizeof(frag_pool_elem_t) * res_len);
	
	for (i = 0; i < res_len; i++)
	{
		p->e[i].data = allocator_malloc(size);
		p->e[i].free = true;
	}
	
	return p;
}

void * frag_pool_alloc (frag_pool_t * p)
{
	register uint i;
	
	for (i = 0; i < p->real_len; i++)
		if (p->e[i].free)
		{
			p->cur_len++;
			p->e[i].free = false;
			return p->e[i].data;
		}
	
	p->cur_len++;
	p->real_len++;
	p->e = (frag_pool_elem_t *) allocator_realloc(p->e, sizeof(frag_pool_elem_t) * p->real_len);
	p->e[p->real_len - 1].data = allocator_malloc(p->node_size);
	p->e[p->real_len - 1].free = false;
	
	return p->e[p->real_len - 1].data;
}

static inline void _frag_try_internal_pool_free (frag_pool_t * p, uint i)
{
	if (i >= p->reserved_len)
	{
		if (i == p->real_len - 1)
		{
			p->real_len--;
			allocator_free(p->e[i].data);
			p->e = (frag_pool_elem_t *) allocator_realloc(p->e, sizeof(frag_pool_elem_t) * p->real_len);
			
			if (i > 0 && p->e[i - 1].free)
				_frag_try_internal_pool_free(p, i - 1);
			
			return;
		}
	}
}

void frag_pool_free_alt (frag_pool_t * p, uint i)
{
	if (!(p->e[i].free))
	{
		p->cur_len--;
		p->e[i].free = true;
		_frag_try_internal_pool_free(p, i);
	}
}

void frag_pool_free (frag_pool_t * p, void * ptr)
{
	register uint i;
	
	for (i = 0; i < p->real_len; i++)
		if (!(p->e[i].free) && p->e[i].data == ptr)
		{
			p->cur_len--;
			p->e[i].free = true;
			_frag_try_internal_pool_free(p, i);
			
			return;
		}
}

pqueue_t * pqueue_create (ulong res_len)
{
	register pqueue_t * p;
	
	p = (pqueue_t *) allocator_malloc(sizeof(pqueue_t));
	p->cur_len = 0;
	p->reserved_len = res_len;
	p->data = allocator_malloc(sizeof(void *) * res_len);
	
	return p;
}

void pqueue_push (pqueue_t * p, void * ptr)
{
	p->cur_len++;
	
	if (p->cur_len > p->reserved_len)
		p->data = allocator_realloc(p->data, sizeof(void *) * p->cur_len);
	
	p->data[p->cur_len - 1] = ptr;
}

void * pqueue_fetch (pqueue_t * p)
{
	p->cur_len--;
	
	if (p->cur_len > p->reserved_len)
		p->data = allocator_realloc(p->data, sizeof(void *) * p->cur_len);
	
	return p->data[p->cur_len];
}

buf_t * buf_create (uint size, uint res_len)
{
	register buf_t * b;
	
	b = (buf_t *) allocator_malloc(sizeof(buf_t));
	b->cur_len = 0;
	b->reserved_len = res_len;
	b->node_size = size;
	b->data = allocator_malloc(size * res_len);
	
	return b;
}

void buf_destroy (buf_t * b)
{
	allocator_free(b->data);
	allocator_free(b);
}

long buf_expand (buf_t * b, uint add)
{
	register void * old_ptr;
	
	b->cur_len += add;
	
	if (b->cur_len > b->reserved_len)
	{
		old_ptr = b->data;
		b->data = allocator_realloc(b->data, b->node_size * b->cur_len);
		
		return (long) ((uchar *) b->data - (uchar *) old_ptr);
	}
	
	return 0;
}

long buf_expand_i (buf_t * b, int add)
{
	register void * old_ptr;
	
	b->cur_len += add;
	
	if (b->cur_len > b->reserved_len)
	{
		old_ptr = b->data;
		b->data = allocator_realloc(b->data, b->node_size * b->cur_len);
		
		return (long) ((uchar *) b->data - (uchar *) old_ptr);
	}
	
	return 0;
}

long buf_resize (buf_t * b, uint new_size)
{
	register void * old_ptr;
	
	b->cur_len = new_size;
	
	if (b->cur_len > b->reserved_len)
	{
		old_ptr = b->data;
		b->data = allocator_realloc(b->data, b->node_size * b->cur_len);
		
		return (long) ((uchar *) b->data - (uchar *) old_ptr);
	}
	
	return 0;
}

void buf_free (buf_t * b)
{
	if (b->cur_len > b->reserved_len)
		b->data = allocator_realloc(b->data, b->node_size * b->reserved_len);
	
	b->cur_len = 0;
}

char * str_reverse (char * str)
{
	size_t i, len, half_len;
	char temp;
	
	len = strlen(str);
	half_len = len / 2;
	len--;
	
	for (i = 0; i < half_len; i++)
	{
		temp = str[i];
		str[i] = str[len - i];
		str[len - i] = temp;
	}
	
	return str;
}

int digits_in_int (int n)
{
	int i;
	
	for (i = 1; (n /= 10); i++);
	
	return i;
}

static const char * x_to_str_chartable = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

void int_to_str (int value, char * result, int base)
{
	register char * ptr = result, * ptr1 = result, tmp_char;
	register int tmp_value;
	
	do
	{
		tmp_value = value;
		value /= base;
		* (ptr++) = x_to_str_chartable[35 + (tmp_value - value * base)];
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

void long_to_str (long value, char * result, int base)
{
	register char * ptr = result, * ptr1 = result, tmp_char;
	register long tmp_value;
	
	do
	{
		tmp_value = value;
		value /= base;
		* (ptr++) = x_to_str_chartable[35 + (tmp_value - value * base)];
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

void int64_to_str (int64 value, char * result, int base)
{
	register char * ptr = result, * ptr1 = result, tmp_char;
	register int64 tmp_value;
	
	do
	{
		tmp_value = value;
		value /= base;
		* (ptr++) = x_to_str_chartable[35 + (tmp_value - value * base)];
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

void str_to_lower (char * str)
{
	for (; * str != '\0'; str++)
		* str = TO_LOWER(* str);
}

bool is_num (char * str)
{
	if (* str == '-')
		str++;
	
	for (; * str != '\0'; str++)
		if (!IS_DIGIT(* str))
			return false;
	
	return true;
}

bool load_file_contents (const char * path, str_big_t * out)
{
	char * content;
	int64 size;
	int fd = open(path, O_RDONLY);
	
	if (fd == -1)
		return false;
	
	#ifdef HAVE_LSEEKI64
	size = _lseeki64(fd, 0L, SEEK_END);
	(void) _lseeki64(fd, 0L, SEEK_SET);
	#else
	size = lseek(fd, 0, SEEK_END);
	(void) lseek(fd, 0, SEEK_SET);
	#endif
	
	if (size == (int64) -1)
	{
		close(fd);
		return false;
	}
	
	content = (char *) allocator_malloc(size + 1);
	
	if (read(fd, content, size) == -1)
	{
		allocator_free(content);
		close(fd);
		return false;
	}
	
	close(fd);
	content[size] = '\0';
	
	out->str = content;
	out->len = size;
	
	return true;
}

bool is_file_exists (const char * path)
{
	struct stat st;
	if (stat(path, &st) == -1)
		return false;
	if (S_ISREG(st.st_mode))
		return true;
	return false;
}

bool is_directory_exists (const char * path, enum io_access_rights ar)
{
	struct stat st;
	if (stat(path, &st) == -1)
		return false;
	if (S_ISDIR(st.st_mode))
	{
		switch (ar)
		{
			case IOAR_FULL:
				if ((st.st_mode & (S_IRUSR | S_IWUSR | S_IXUSR)) == (S_IRUSR | S_IWUSR | S_IXUSR))
					return true;
				break;
			case IOAR_RW:
				if ((st.st_mode & (S_IRUSR | S_IWUSR)) == (S_IRUSR | S_IWUSR))
					return true;
				break;
			case IOAR_W:
				if ((st.st_mode & S_IWUSR) == S_IWUSR)
					return true;
				break;
			case IOAR_R:
				if ((st.st_mode & S_IRUSR) == S_IRUSR)
					return true;
				break;
			case IOAR_NONE:
				return true;
		}
	}
	return false;
}

bool is_node_exists (const char * path)
{
	struct stat st;
	if (stat(path, &st) == -1)
		return false;
	return true;
}

bool is_path_absolute (const char * path)
{
	size_t len = strlen(path);
	
	if (len > 1 && path[0] == '/')
		return true;
	#ifdef _WIN
	if (len > 3 && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
		return true;
	#endif
	
	return false;
}

#ifdef _WIN
char * _getcwd (char *, int);
#endif

char * gnu_getcwd (void)
{
	#ifdef _WIN
	return _getcwd(NULL, 0);
	#else
	return getcwd(NULL, 0);
	#endif
}

/* Save the location of the current working directory for the purpose of returning to it later */
umlong save_cwd (void)
{
	#ifdef _WIN
	return (umlong) gnu_getcwd();
	#else
	return (umlong) open(".", O_RDONLY);
	#endif
}

bool restore_cwd (umlong d)
{
	#ifdef _WIN
	int ret = _chdir((const char *) d);
	free((void *) d);
	
	return ((ret == -1) ? false : true);
	#else
	int ret;
	
	if (likely(d != -1))
	{
		ret = fchdir(d);
		close(d);
		
		if (unlikely(ret == -1))
			return false;
		
		return true;
	}
	
	return false;
	#endif
}

const char * gnu_basename (const char * path)
{
	register char * p = (char *) path + strlen(path);
	
	if (p == path)
		return p;
	
	for (p--; p != path; p--)
		#ifdef _WIN
		if (* p == '/' || * p == '\\')
		#else
		if (* p == '/')
		#endif
		{
			p++;
			break;
		}
	
	return p;
}
