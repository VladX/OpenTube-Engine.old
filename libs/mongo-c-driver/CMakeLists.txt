project (mongocdriver C)
cmake_minimum_required (VERSION 2.6)

set (MONGOCDRIVER_LIBRARY_SRC_LIST src/md5.c src/mongo.c src/gridfs.c src/bson.c src/numbers.c src/encoding.c src/net.c)

include (TestBigEndian)

test_big_endian (IS_HOST_BIG_ENDIAN)

if (IS_HOST_BIG_ENDIAN)
 set (MONGO_BIG_ENDIAN 1)
endif (IS_HOST_BIG_ENDIAN)

set (MONGO_HAVE_STDINT 1)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
 set (CLANG_COMPILER 1)
endif ("${CMAKE_C_COMPILER_ID}" MATCHES "clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")

if (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN OR CLANG_COMPILER)
 set (CMAKE_C_FLAGS "-O3 -g -Wall")
endif (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN OR CLANG_COMPILER)

if (MSVC)
 set (CMAKE_C_FLAGS "/MT /O2")
endif (MSVC)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
 set (_MONGO_USE_LINUX_SYSTEM 1)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux")

if (MSVC OR MINGW OR CMAKE_SYSTEM_NAME MATCHES "Windows")
 set (_MONGO_WIN 1)
endif (MSVC OR MINGW OR CMAKE_SYSTEM_NAME MATCHES "Windows")

configure_file (src/config.h.in src/config.h)

include_directories (src)
add_library (mongocdriver STATIC ${MONGOCDRIVER_LIBRARY_SRC_LIST})
