#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "common_types.h"
#include "global.h"

#define E_COMMON_FUNCTIONS_H


#define err_f(FILE, FMT, ...) fprintf(FILE, FMT, __VA_ARGS__)
#define err(FMT, ...) err_f(stderr, "(File \"%s\", line %d): " FMT "\n", __FILE__, __LINE__, __VA_ARGS__)
#define perr(FMT, ...) ((errno) ? err(FMT ": %s", __VA_ARGS__, strerror(errno)) : err(FMT, __VA_ARGS__))
#define eerr(EXIT_CODE, ...) { err(__VA_ARGS__); exit(EXIT_CODE); }
#define peerr(EXIT_CODE, ...) { perr(__VA_ARGS__); exit(EXIT_CODE); }

#define _debug_print(LEVEL, FMT, ...) printf("(Debug level %d, file \"%s\", line %d): " FMT "\n", LEVEL, __FILE__, __LINE__, __VA_ARGS__)

#define debug_print_0(FMT, ...) _debug_print(0, FMT, __VA_ARGS__)

#if DEBUG_LEVEL > 0
 #define debug_print_1(FMT, ...) _debug_print(1, FMT, __VA_ARGS__)
#else
 #define debug_print_1(FMT, ...)
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

#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))

#define IS_SYM(X) ((X) >= ' ' && (X) <= '~')
#define TO_LOWER(X) (((X) >= 'A' && (X) <= 'Z') ? (X) + 32 : (X))
#define IS_SPACE(X) ((X) == ' ' || (X) == '\t' || (X) == '\n')
#define IS_DIGIT(X) ((X) >= '0' && (X) <= '9')
#define IS_VALID_PATH_CHARACTER(X) (((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z') || ((X) >= '0' && (X) <= '9') || (X) == '/' || (X) == '-' || (X) == '_' || (X) == '.' || (X) == ',' || (X) == ':')

void set_str (str_t * str, char * src);

void set_cpy_str (str_t * str, char * src);

str_t * new_str (char * src);

pool_t * pool_create (uint size, uint start_len);

void * pool_alloc (pool_t * p);

void pool_free (pool_t * p, uint len);

void pool_free_last (pool_t * p, uint len);

buf_t * buf_create (ulong size, ulong res_len);

void buf_destroy (buf_t * b);

void buf_expand (buf_t * b, uint add);

void buf_free (buf_t * b);

void int_to_str (int value, char * result, int base);

bool is_num (char * str);

bool smemcmp (uchar * p1, uchar * p2, uint len);
