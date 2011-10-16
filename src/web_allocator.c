/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common_functions.h"
#include "web_handler.h"

struct web_alloc
{
	void * ptr;
	bool free;
};

static threadsafe buf_t * alloc_list;

void * web_allocator_malloc (size_t size)
{
	void * ptr = allocator_malloc(size);
	struct web_alloc * a;
	
	if (unlikely(ptr == NULL))
		web_raise(500);
	
	buf_expand(alloc_list, 1);
	a = &(((struct web_alloc *) alloc_list->data)[alloc_list->cur_len - 1]);
	a->ptr = ptr;
	a->free = false;
	
	return ptr;
}

void web_allocator_free (void * ptr)
{
	struct web_alloc * a;
	uint i;
	
	for (i = 0; i < alloc_list->cur_len; i++)
	{
		a = &(((struct web_alloc *) alloc_list->data)[i]);
		
		if (ptr == a->ptr && a->free == false)
		{
			allocator_free(ptr);
			a->free = true;
			
			return;
		}
	}
}

void web_allocator_garbage_collect (void)
{
	struct web_alloc * a;
	uint i;
	
	for (i = 0; i < alloc_list->cur_len; i++)
	{
		a = &(((struct web_alloc *) alloc_list->data)[i]);
		
		if (a->free == false)
			allocator_free(a->ptr);
	}
	
	buf_free(alloc_list);
}

void web_allocator_init (void)
{
	alloc_list = buf_create(sizeof(struct web_alloc), WEB_ALLOCATOR_LIST_RESERVED_SIZE);
}
