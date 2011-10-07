project (mongocdriver C)
cmake_minimum_required (VERSION 2.6)

set (MONGOCDRIVER_LIBRARY_SRC_LIST src/md5.c src/mongo.c src/gridfs.c src/bson.c src/numbers.c src/encoding.c)

include (TestBigEndian)

test_big_endian (IS_HOST_BIG_ENDIAN)

if (IS_HOST_BIG_ENDIAN)
 add_definitions (-DMONGO_BIG_ENDIAN)
endif (IS_HOST_BIG_ENDIAN)

add_definitions (-DMONGO_HAVE_STDINT)

if (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)
 set (CMAKE_C_FLAGS "-O3 -g -Wall")
endif (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)

if (MSVC)
 set (CMAKE_C_FLAGS "/MT /O2")
endif (MSVC)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
 add_definitions (-D_MONGO_USE_LINUX_SYSTEM)
 set (MONGOCDRIVER_LIBRARY_SRC_LIST ${MONGOCDRIVER_LIBRARY_SRC_LIST} src/platform/linux/net.c)
else (CMAKE_SYSTEM_NAME MATCHES "Linux")
 set (MONGOCDRIVER_LIBRARY_SRC_LIST ${MONGOCDRIVER_LIBRARY_SRC_LIST} src/net.c)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux")

include_directories (src)
add_library (mongocdriver STATIC ${MONGOCDRIVER_LIBRARY_SRC_LIST})
