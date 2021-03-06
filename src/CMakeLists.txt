include (CheckTypeSize)

set (DEBUG_LEVEL "0" CACHE STRING "Debug level.")
set (DEBUG_CCFLAGS "" CACHE STRING "Debug flags.")
set (CMAKE_C_FLAGS_DEBUG ${DEBUG_CCFLAGS} CACHE STRING "Debug flags." FORCE)
set (CMAKE_CXX_FLAGS_DEBUG ${DEBUG_CCFLAGS} CACHE STRING "Debug flags." FORCE)
set (CCFLAGS "")
set (CMAKE_C_FLAGS_RELEASE ${CCFLAGS} CACHE STRING "Release flags." FORCE)
set (CMAKE_CXX_FLAGS_RELEASE ${CCFLAGS} CACHE STRING "Release flags." FORCE)
set (CMAKE_C_FLAGS "" CACHE STRING "Flags used by the C compiler during all build types." FORCE)
set (CMAKE_CXX_FLAGS "" CACHE STRING "Flags used by the CXX compiler during all build types." FORCE)
set (CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

#### Optional features ####

option (BUILD_INSTALLER "Build installer for windows." OFF)
option (ENABLE_SUPPORT_SPIDERMONKEY "Support for SpiderMonkey Javascript engine." OFF)
set (IPv6_SUPPORT "1" CACHE STRING "Support for IPv6 (1 or 0).")

###########################

if (NOT CMAKE_BUILD_TYPE)
 set (CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set (link_libraries_list pthread z pcre jemalloc mongocdriver)
set (link_directories_list ${CMAKE_BINARY_DIR}/libs/jemalloc)
set (exec_filename opentube-server)

check_type_size ("void *" TYPE_VOID_POINTER_SIZE BUILTIN_TYPES_ONLY)
math (EXPR COMPILER_TARGET_BITNESS "${TYPE_VOID_POINTER_SIZE} * 8")
message (STATUS "Compilier is ${COMPILER_TARGET_BITNESS}-bit")

if (WIN32 OR MINGW)
 include_directories (AFTER ${CMAKE_CURRENT_SOURCE_DIR}/libs/win32/include)
 set (CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/libs/win32/include)
 set (link_directories_list ${link_directories_list} ${CMAKE_BINARY_DIR}/libs/win${COMPILER_TARGET_BITNESS})
 set (link_libraries_list ${link_libraries_list} ws2_32 advapi32 netapi32 mswsock)
 set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ws2_32)
else (WIN32 OR MINGW)
 set (link_libraries_list ${link_libraries_list} crypto)
endif (WIN32 OR MINGW)

if (MINGW)
 set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static")
 set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static")
 set (CCFLAGS "${CCFLAGS} -DWIN32_LEAN_AND_MEAN -DWIN32 -enable-stdcall-fixup")
endif (MINGW)

if (MSVC)
 set (CCFLAGS "${CCFLAGS} /MT /O2 /DWIN32_LEAN_AND_MEAN /DWIN32 ${DEBUG_CCFLAGS}")
 set (CMAKE_C_FLAGS "${CCFLAGS}")
 set (CMAKE_CXX_FLAGS "${CCFLAGS} /EHsc")
else (MSVC)
 set (link_libraries_list ${link_libraries_list} m)
endif (MSVC)

if (CMAKE_SYSTEM_NAME MATCHES ".*BSD")
 set (_BSD 1)
endif (CMAKE_SYSTEM_NAME MATCHES ".*BSD")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
 set (_LINUX 1)
endif (CMAKE_SYSTEM_NAME MATCHES "Linux")

if (NOT _LINUX OR MINGW)
 set (link_libraries_list ${link_libraries_list} intl iconv)
endif (NOT _LINUX OR MINGW)

if (_LINUX)
 set (CCFLAGS "${CCFLAGS} -pthread")
endif (_LINUX)

add_subdirectory (test_features test_features)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

if (PYTHON_EXECUTABLE)
 execute_process (COMMAND ${PYTHON_EXECUTABLE} callbacks/generate-callbacks.c.py ${CMAKE_CURRENT_SOURCE_DIR}/web/ WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web)
 message (STATUS "Generated callbacks.c.")
 execute_process (COMMAND ${PYTHON_EXECUTABLE} update-strings.h.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lang)
 message (STATUS "Generated strings.h.")
endif (PYTHON_EXECUTABLE)

if (HAVE_SPIDERMONKEY_ENGINE AND SPIDERMONKEY_ENGINE_LIBRARY)
 include_directories (AFTER ${HAVE_SPIDERMONKEY_ENGINE})
 set (link_libraries_list ${link_libraries_list} ${SPIDERMONKEY_ENGINE_LIBRARY})
endif (HAVE_SPIDERMONKEY_ENGINE AND SPIDERMONKEY_ENGINE_LIBRARY)

file (GLOB source_files *.c*)
file (STRINGS web/compile.list source_files_web)

foreach (source_file ${source_files_web})
 set (source_files ${source_files} web/${source_file})
endforeach (source_file)

if (COMPILER_HAVE_LTO AND COMPILER_HAVE_USE_LINKER_PLUGIN AND COMPILER_HAVE_OFAST)
 set (CCFLAGS "${CCFLAGS} -fuse-linker-plugin -flto")
endif (COMPILER_HAVE_LTO AND COMPILER_HAVE_USE_LINKER_PLUGIN AND COMPILER_HAVE_OFAST)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
 set (CLANG_COMPILER 1)
endif ("${CMAKE_C_COMPILER_ID}" MATCHES "clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")

if (CLANG_COMPILER)
 set (CCFLAGS "${CCFLAGS} -Wall -Wunused -Wundef -Wpointer-arith -Wtype-limits -O2 ${DEBUG_CCFLAGS}")
 set (CMAKE_C_FLAGS "${CCFLAGS} -std=gnu89")
 set (CMAKE_CXX_FLAGS "${CCFLAGS}")
endif (CLANG_COMPILER)

if (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)
 set (CCFLAGS "${CCFLAGS} -Wall -Wunused -Wundef -Wpointer-arith -Wtype-limits -O2 ${DEBUG_CCFLAGS}")
 set (CMAKE_C_FLAGS "${CCFLAGS}")
 set (CMAKE_CXX_FLAGS "${CCFLAGS}")
 set_property (SOURCE web/utils/captcha.c PROPERTY COMPILE_FLAGS "${CCFLAGS} -fno-math-errno -funsafe-math-optimizations -fno-trapping-math")
endif (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)

include_directories (BEFORE ./)
include_directories (BEFORE ${CMAKE_SOURCE_DIR}/libs/mongo-c-driver/src/)
link_directories (${link_directories_list})
add_executable (${exec_filename} ${source_files})
target_link_libraries (${exec_filename} ${link_libraries_list})

if (BUILD_INSTALLER AND (WIN32 OR MINGW))
 add_subdirectory (windows_installer)
endif (BUILD_INSTALLER AND (WIN32 OR MINGW))
