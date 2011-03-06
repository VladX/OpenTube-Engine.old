set (DEBUG_LEVEL "0" CACHE STRING "Debug level")
set (DEBUG_CCFLAGS "" CACHE STRING "Debug flags")
option (GENERATE_CALLBACKS "Generate new callbacks.c from web/ tree")

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
add_executable (opentube-server ${source_files})
target_link_libraries (opentube-server pthread z pcre)
