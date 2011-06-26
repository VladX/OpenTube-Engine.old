project (opentube-installer)
cmake_minimum_required (VERSION 2.6)

set (exec_filename setup.exe)
set (link_libraries_list ssl crypto tar)
set (link_directories_list)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)
find_package (Qt4 REQUIRED)
set (QT_USE_QTMAIN TRUE)
set (QT_USE_QTNETWORK TRUE)
include (${QT_USE_FILE})
file (GLOB source_files *.cpp)
file (GLOB headers *.h)

qt4_add_resources (resources ${CMAKE_CURRENT_SOURCE_DIR}/../../res/windows_installer/resources.qrc)
qt4_wrap_cpp (moc_sources ${headers})

if (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)
 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Os -UQT_DLL")
 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Os -UQT_DLL")
endif (CMAKE_COMPILER_IS_GNUCC OR MINGW OR CYGWIN)

if (MINGW)
 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -Wl,-subsystem,windows")
 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -Wl,-subsystem,windows")
endif (MINGW)

if (MSVC)
 set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /O1 /UQT_DLL")
 set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O1 /UQT_DLL /EHsc")
endif (MSVC)

if (WIN32 OR MINGW)
 include_directories (AFTER ${CMAKE_CURRENT_SOURCE_DIR}/../libs/win32/include)
 set (link_directories_list "${CMAKE_CURRENT_BINARY_DIR}/../../libs/win32")
 set (link_libraries_list ${link_libraries_list} ole32 gdi32 ws2_32 winspool imm32 comdlg32 uuid)
endif (WIN32 OR MINGW)

link_directories (${link_directories_list} ${LINK_DIRECTORIES})
add_executable (${exec_filename} WIN32 ${moc_sources} ${resources} ${source_files})
target_link_libraries (${exec_filename} ${QT_LIBRARIES} ${QT_QTMAIN_LIBRARY} ${link_libraries_list})
