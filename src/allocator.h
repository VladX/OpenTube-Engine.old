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

#ifndef __ALLOCATOR_H
#define __ALLOCATOR_H

#define ALLOCATOR_MALLOC_FN je_malloc
#define ALLOCATOR_CALLOC_FN je_calloc
#define ALLOCATOR_REALLOC_FN je_realloc
#define ALLOCATOR_MEMALIGN_FN je_memalign
#define ALLOCATOR_FREE_FN je_free

void * ALLOCATOR_CALLOC_FN (size_t nmemb, size_t size);
void * ALLOCATOR_MALLOC_FN (size_t size);
void ALLOCATOR_FREE_FN (void * ptr);
void * ALLOCATOR_REALLOC_FN (void * ptr, size_t size);
void * ALLOCATOR_MEMALIGN_FN (size_t boundary, size_t size);

#ifdef _WIN
bool malloc_init_hard (void);
 #define allocator_init() malloc_init_hard()
#else
 #define allocator_init()
#endif

#define allocator_malloc(SIZE) ALLOCATOR_MALLOC_FN(SIZE)
#define allocator_calloc(NMEMB, SIZE) ALLOCATOR_CALLOC_FN(NMEMB, SIZE)
#define allocator_realloc(PTR, SIZE) ALLOCATOR_REALLOC_FN(PTR, SIZE)
#define allocator_memalign(BOUNDARY, SIZE) ALLOCATOR_MEMALIGN_FN(BOUNDARY, SIZE)
#define allocator_free(PTR) ALLOCATOR_FREE_FN((void *) PTR)

/* FIXME Using our allocator in new/delete operators can lead to memory corruption when using third-party C++ libs */
#if 0 && defined(__cplusplus)

/* Overload new/delete operators */

extern "C++"
{

inline void * operator new (size_t size)
{
	void * ptr = (void *) allocator_malloc(size);
	
	if (ptr == NULL)
		throw std::bad_alloc();
	
	return ptr;
}

inline void * operator new [] (size_t size)
{
	void * ptr = (void *) allocator_malloc(size);
	
	if (ptr == NULL)
		throw std::bad_alloc();
	
	return ptr;
}

inline void operator delete (void * ptr)
{
	allocator_free(ptr);
}

inline void operator delete [] (void * ptr)
{
	allocator_free(ptr);
}

inline void * operator new (size_t size, const std::nothrow_t &)
{
	return (void *) allocator_malloc(size);
}

inline void * operator new [] (size_t size, const std::nothrow_t &)
{
	return (void *) allocator_malloc(size);
}

inline void operator delete (void * ptr, const std::nothrow_t &)
{
	allocator_free(ptr);
}

inline void operator delete [] (void * ptr, const std::nothrow_t &)
{
	allocator_free(ptr);
}

}

#endif

#endif
