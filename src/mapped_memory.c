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

#include "common_functions.h"
#include <pthread.h>

#ifndef HAVE_MMAP
 #ifndef HAVE_CREATEFILEMAPPING
  #error "Your OS does not support mmap() or CreateFileMapping()"
 #endif
#endif

#ifdef HAVE_MMAP
 #undef HAVE_CREATEFILEMAPPING
 #include <sys/mman.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #ifndef O_NOATIME
  #define O_NOATIME 0
 #endif
#endif

#ifdef HAVE_CREATEFILEMAPPING
 #include <windows.h>
#endif

#define TMPID_SIZE 10

static pthread_spinlock_t spin[1];
static char tmpid[TMPID_SIZE];

uchar * create_mapped_memory (request_t * r, uint length)
{
	register uchar * mem;
	register char * path = (char *) config.temp_dir.str;
	
	pthread_spin_lock(spin);
	
	int_to_str(r->sock, tmpid, 32);
	memcpy(path + config.temp_dir.len, tmpid, TMPID_SIZE);
	
	#ifdef HAVE_MMAP
	r->temp.file = open(path, O_CREAT | O_NOATIME | O_RDWR, S_IRUSR | S_IWUSR);
	
	pthread_spin_unlock(spin);
	
	if (r->temp.file == -1)
		peerr(-1, "open(%s)", path);
	
	if (ftruncate(r->temp.file, length) == -1)
		peerr(-1, "ftruncate(%d, %u)", r->temp.file, length);
	
	mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, r->temp.file, 0);
	
	if (mem == MAP_FAILED)
		peerr(-1, "mmap(%u)", length);
	#endif
	
	#ifdef HAVE_CREATEFILEMAPPING
	r->temp.hMapFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_RANDOM_ACCESS, NULL);
	
	pthread_spin_unlock(spin);
	
	if (r->temp.hMapFile == INVALID_HANDLE_VALUE)
		peerr(-1, "CreateFileA(%s)", path);
	
	r->temp.hMap = CreateFileMapping(r->temp.hMapFile, NULL, PAGE_READWRITE, 0, length, NULL);
	
	if (r->temp.hMap == NULL)
		peerr(-1, "CreateFileMapping(%u)", length);
	
	mem = MapViewOfFile(r->temp.hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	
	if (mem == NULL)
		peerr(-1, "%s", "MapViewOfFile()");
	#endif
	
	return mem;
}

void destroy_mapped_memory (request_t * r, uint length, uchar * ptr)
{
	#ifdef HAVE_MMAP
	if (munmap(ptr, length) == -1)
		perr("munmap(%u)", length);
	
	if (ftruncate(r->temp.file, 0) == -1)
		perr("ftruncate(%d, %u)", r->temp.file, 0U);
	
	close(r->temp.file);
	#endif
	#ifdef HAVE_CREATEFILEMAPPING
	if (!UnmapViewOfFile(ptr))
		perr("UnmapViewOfFile(%p)", ptr);
	
	CloseHandle(r->temp.hMap);
	
	if (SetFilePointer(r->temp.hMapFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		perr("SetFilePointer(%d)", 0);
	
	(void) SetEndOfFile(r->temp.hMapFile);
	CloseHandle(r->temp.hMapFile);
	#endif
}

void mapped_memory_init (void)
{
	pthread_spin_init(spin, PTHREAD_PROCESS_PRIVATE);
}
