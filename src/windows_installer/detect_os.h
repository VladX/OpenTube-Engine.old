#ifndef DETECT_OS_H
#define DETECT_OS_H 1

#undef OS_WINDOWS

#if defined(_MSC_VER) || defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
 #define OS_WINDOWS 1
#endif

#endif
