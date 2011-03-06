include (CheckFunctionExists.cmake)
include (CheckIncludeFile.cmake)

macro (required_lib variable fileinclude libname)
 check_include_file (${fileinclude} ${variable})
 if (NOT ${variable})
  message (FATAL_ERROR "Required library ${libname} not found.")
 endif (NOT ${variable})
endmacro (required_lib)

check_function_exists (accept4 HAVE_ACCEPT4)
required_lib (HAVE_ZLIB_H zlib.h zlib)
required_lib (HAVE_PCRE_H pcre.h PCRE)
