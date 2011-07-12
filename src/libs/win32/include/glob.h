/* Windows wrapper for POSIX glob() */

#ifndef _GLOB_H
#define _GLOB_H 1

typedef struct
{
	int gl_pathc;
	char ** gl_pathv;
} glob_t;

/* Error returns from `glob'.  */
#define GLOB_NOSPACE 1
#define GLOB_ABORTED 2
#define GLOB_NOMATCH 3
#define GLOB_NOSYS 4

#endif
