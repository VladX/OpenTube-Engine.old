/* Windows wrapper for POSIX glob() */

#ifndef _GLOB_H
#define _GLOB_H 1

typedef struct
{
	int gl_pathc;
	char ** gl_pathv;
} glob_t;

#endif
