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

#ifndef E_COMMON_FUNCTIONS_H
#define E_COMMON_FUNCTIONS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config.h"

#ifdef _WIN
 #include <windows.h>
#endif

#ifdef HAVE_MALLOC_H
 #include <malloc.h>
#endif

#include "common_types.h"
#include "allocator.h"
#include "global.h"
#include "localization.h"
#include "core_logger.h"


#define err_f(FILE, FMT, ...) fprintf(FILE, FMT, __VA_ARGS__)
#define log_msg(FMT, ...) logger_log("(File \"%s\", line %d): " FMT, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)
#define err(FMT, ...) logger_log_error("(File \"%s\", line %d): " FMT, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)
#define perr(FMT, ...) logger_log_perror("(File \"%s\", line %d): " FMT, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)
#define eerr(EXIT_CODE, FMT, ...) logger_log_critical(EXIT_CODE, "(File \"%s\", line %d): " FMT, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)
#define peerr(EXIT_CODE, FMT, ...) logger_log_pcritical(EXIT_CODE, "(File \"%s\", line %d): " FMT, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)

#define _debug_print(LEVEL, FMT, ...) logger_log("(Debug level %d, file \"%s\", line %d): " FMT, LEVEL, gnu_basename(__FILE__), __LINE__, __VA_ARGS__)

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

#define _BEGIN_LOCAL_SECTION_ {
#define _END_LOCAL_SECTION_ }

#ifdef __GNUC__
 #define threadsafe __thread
#else
 #define threadsafe __declspec(thread)
#endif

#ifdef _MSVC_
 #define lround(value) ((long) floor((value) + 0.5))
 #define inline __inline
#endif

#ifdef COMPILER_HAVE_BUILTIN_EXPECT
 #define likely(X) __builtin_expect(!!(X), 1)
 #define unlikely(X) __builtin_expect(!!(X), 0)
#else
 #define likely(X) (X)
 #define unlikely(X) (X)
#endif

/* Platform-specific */
#ifdef _WIN
 #define socket_close(SOCKET) closesocket(SOCKET)
 #define socket_wouldblock(ERROR_CODE) (ERROR_CODE == WSAEWOULDBLOCK)
 #define socket_wouldntblock(ERROR_CODE) (ERROR_CODE != WSAEWOULDBLOCK)
 #define getpid (int) GetCurrentProcessId
 #undef errno
 #define errno WSAGetLastError()
 #define socket_errno WSAGetLastError()
 #define io_errno GetLastError()
#else
 #define socket_close(SOCKET) close(SOCKET)
 #define socket_wouldblock(ERROR_CODE) (ERROR_CODE == EAGAIN)
 #define socket_wouldntblock(ERROR_CODE) (ERROR_CODE != EAGAIN)
 #define socket_errno errno
 #define io_errno errno
#endif

#if defined(HAVE_STRTOI64) && !defined(HAVE_STRTOLL) && !defined(strtoll)
 #define strtoll(X, Y, Z) _strtoi64(X, Y, Z)
#endif

#if defined(HAVE_STRICMP) && !defined(HAVE_STRCASECMP) && !defined(strcasecmp)
 #define strcasecmp(X, Y) _stricmp(X, Y)
#endif

#if defined(HAVE_STRNICMP) && !defined(HAVE_STRNCASECMP) && !defined(strncasecmp)
 #define strncasecmp(X, Y, Z) _strnicmp(X, Y, Z)
#endif

void set_str (str_t * str, const char * src);

void set_cpy_str (str_t * str, const char * src);

str_t * new_str (const char * src);

void set_ustr (u_str_t * str, const uchar * src);

void set_cpy_ustr (u_str_t * str, const uchar * src);

u_str_t * new_ustr (const uchar * src);

pool_t * pool_create (uint size, uint start_len);

void * pool_alloc (pool_t * p);

void pool_free (pool_t * p, uint len);

void pool_free_last (pool_t * p, uint len);

void pool_destroy (pool_t * p);

frag_pool_t * frag_pool_create (uint size, uint res_len);

void * frag_pool_alloc (frag_pool_t * p);

void frag_pool_free (frag_pool_t * p, void * ptr);

void frag_pool_free_alt (frag_pool_t * p, uint i);

pqueue_t * pqueue_create (ulong res_len);

void pqueue_push (pqueue_t * p, void * ptr);

void * pqueue_fetch (pqueue_t * p);

buf_t * buf_create (uint size, uint res_len);

void buf_destroy (buf_t * b);

intptr_t buf_expand (buf_t * b, uint add);

intptr_t buf_expand_i (buf_t * b, int add);

intptr_t buf_resize (buf_t * b, uint new_size);

void buf_free (buf_t * b);

char * str_reverse (char * str);

int digits_in_int (int n);

void int_to_str (int value, char * result, int base);

void long_to_str (long value, char * result, int base);

void int64_to_str (int64 value, char * result, int base);

void str_to_lower (char * str);

bool is_num (char * str);

bool load_file_contents (const char * path, str_big_t * out);

bool is_file_exists (const char * path);

bool is_directory_exists (const char * path, enum io_access_rights ar);

bool is_node_exists (const char * path);

bool is_path_absolute (const char * path);

char * gnu_getcwd (void);

umlong save_cwd (void);

umlong save_and_change_cwd (const char * path);

bool restore_cwd (umlong d);

const char * gnu_basename (const char * path);

#endif
