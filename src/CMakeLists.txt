set (DEBUG_LEVEL "0" CACHE STRING "Debug level")
set (DEBUG_CCFLAGS "" CACHE STRING "Debug flags")
option (GENERATE_CALLBACKS "Generate new callbacks.c from web/ tree")

set (link_libraries_list pthread z pcre)
set (exec_filename opentube-server)

if (WIN32 OR MINGW)
 include_directories (AFTER ${CMAKE_CURRENT_SOURCE_DIR}/libs/win32/include)
 set (CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/libs/win32/include)
 link_directories (${CMAKE_BINARY_DIR}/libs/win32)
 set (link_libraries_list ${link_libraries_list} ws2_32)
 set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ws2_32)
 set (exec_filename opentube-server.exe)
endif (WIN32 OR MINGW)

if (MINGW)
 set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static")
 set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static")
endif (MINGW)

add_subdirectory (test_features test_features)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

if (GENERATE_CALLBACKS)
 find_program (python_path python)
 if (python_path)
  execute_process (COMMAND ${python_path} callbacks/generate-callbacks.c.py ${CMAKE_CURRENT_SOURCE_DIR}/web/ WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web)
 endif (python_path)
endif (GENERATE_CALLBACKS)

file (GLOB source_files *.c)
file (STRINGS web/compile.list source_files_web)

foreach (source_file ${source_files_web})
 set (source_files ${source_files} web/${source_file})
endforeach (source_file)

if (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)
 set (CCFLAGS "-Wall -Wunused -Wundef -Wpointer-arith -O2 ${DEBUG_CCFLAGS}")
 set (CMAKE_C_FLAGS "${CCFLAGS}")
 #set_property (SOURCE memcache.c PROPERTY COMPILE_FLAGS "${CCFLAGS}")
endif (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)

include_directories (BEFORE ./)
add_executable (${exec_filename} ${source_files})
target_link_libraries (${exec_filename} ${link_libraries_list})
