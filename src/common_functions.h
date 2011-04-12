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

#ifndef E_COMMON_FUNCTIONS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN
 #include <windows.h>
#endif
#include "config.h"
#include "common_types.h"
#include "global.h"
#include "localization.h"

#define E_COMMON_FUNCTIONS_H 1


#define err_f(FILE, FMT, ...) fprintf(FILE, FMT, __VA_ARGS__)
#define err(FMT, ...) err_f(stderr, "(File \"%s\", line %d): " FMT "\n", __FILE__, __LINE__, __VA_ARGS__)
#define perr(FMT, ...) ((errno) ? err(FMT ": %s", __VA_ARGS__, strerror(errno)) : err(FMT, __VA_ARGS__))
#define eerr(EXIT_CODE, ...) { err(__VA_ARGS__); exit(EXIT_CODE); }
#define peerr(EXIT_CODE, ...) { perr(__VA_ARGS__); exit(EXIT_CODE); }

#define _debug_print(LEVEL, FMT, ...) printf("(Debug level %d, file \"%s\", line %d): " FMT "\n", LEVEL, __FILE__, __LINE__, __VA_ARGS__)

#define debug_print_0(FMT, ...) _debug_print(0, FMT, __VA_ARGS__)

#if DEBUG_LEVEL > 0
 #define debug_print_1(FMT, ...) _debug_print(1, FMT, __VA_ARGS__)
 #include <assert.h>
#else
 #define debug_print_1(FMT, ...)
 #define assert(ignore)
#endif

#if DEBUG_LEVEL > 1
 #define debug_print_2(FMT, ...) _debug_print(2, FMT, __VA_ARGS__)
#else
 #define debug_print_2(FMT, ...)
#endif

#if DEBUG_LEVEL > 2
 #define debug_print_3(FMT, ...) _debug_print(3, FMT, __VA_ARGS__)
#else
 #define debug_print_3(FMT, ...)
#endif

#define ARRAY_LENGTH(X) (sizeof(X) / sizeof(X[0]))

#undef min
#undef max
#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))

#define IS_SYM(X) ((X) >= ' ' && (X) <= '~')
#define TO_LOWER(X) (((X) >= 'A' && (X) <= 'Z') ? (X) + 32 : (X))
#define IS_SPACE(X) ((X) == ' ' || (X) == '\t' || (X) == '\n')
#define IS_DIGIT(X) ((X) >= '0' && (X) <= '9')
#define IS_VALID_PATH_CHARACTER(X) (((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z') || ((X) >= '0' && (X) <= '9') || (X) == '/' || (X) == '-' || (X) == '_' || (X) == '.' || (X) == ',' || (X) == ':' || (X) == '~')
#define IS_VALID_URL_KEY_CHARACTER(X) (((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z') || ((X) >= '0' && (X) <= '9') || (X) == '-' || (X) == '_')
#define IS_VALID_URL_VALUE_CHARACTER(X) (IS_VALID_URL_KEY_CHARACTER(X) || (X) == '+' || (X) == '%')

#ifdef __GNUC__
 #define threadsafe __thread
#else
 #define threadsafe __declspec(thread)
#endif

/* Platform-specific */
#ifdef _WIN
 #define socket_close(SOCKET) closesocket(SOCKET)
 #define getpid (int) GetCurrentProcessId
 #undef errno
 #define errno WSAGetLastError()
 #define socket_errno WSAGetLastError()
 #define io_errno GetLastError()
 #undef peerr
 #define peerr(EXIT_CODE, ...) { err(__VA_ARGS__); win32_fatal_error(""); }
#else
 #define socket_close(SOCKET) close(SOCKET)
 #define socket_errno errno
 #define io_errno errno
#endif

void set_str (str_t * str, char * src);

void set_cpy_str (str_t * str, char * src);

str_t * new_str (char * src);

pool_t * pool_create (uint size, uint start_len);

void * pool_alloc (pool_t * p);

void pool_free (pool_t * p, uint len);

void pool_free_last (pool_t * p, uint len);

frag_pool_t * frag_pool_create (uint size, uint res_len);

void * frag_pool_alloc (frag_pool_t * p);

void frag_pool_free (frag_pool_t * p, void * ptr);

void frag_pool_free_alt (frag_pool_t * p, uint i);

pqueue_t * pqueue_create (ulong res_len);

void pqueue_push (pqueue_t * p, void * ptr);

void * pqueue_fetch (pqueue_t * p);

buf_t * buf_create (uint size, uint res_len);

void buf_destroy (buf_t * b);

long buf_expand (buf_t * b, uint add);

long buf_resize (buf_t * b, uint new_size);

void buf_free (buf_t * b);

void int_to_str (int value, char * result, int base);

void str_to_lower (char * str);

bool is_num (char * str);

bool is_directory_exists (const char * path);

char * gnu_getcwd (void);

#endif
