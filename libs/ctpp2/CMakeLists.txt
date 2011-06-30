#
# CTPP - C++ Template Engine
#
# Cmake configuration file
#
PROJECT(CTPP)

SET(CTPP_VERSION_MAJOR 2)
SET(CTPP_VERSION_MINOR 6)
SET(CTPP_VERSION_PATCH 14)

SET(CTPP_VERSION         "${CTPP_VERSION_MAJOR}.${CTPP_VERSION_MINOR}.${CTPP_VERSION_PATCH}")
SET(CTPP_IDENT           "Dzoraget")
SET(CTPP_MASTER_SITE_URL "http://ctpp.havoc.ru/")

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

OPTION(DEBUG_MODE          "Enable debug output [default: OFF]"                 OFF)
OPTION(ENABLE_OPTIMIZATION "Enable optimization [default: ON]"                  ON)
OPTION(SKIP_RELINK_RPATH   "Skip relinking and full RPATH for the install tree" OFF)

OPTION(ICONV_DISCARD_ILSEQ "Discard illegal sequence and continue (iconv) [default: ON]"    ON)
OPTION(ICONV_TRANSLITERATE "Enable transliteration in the conversion (iconv) [default: ON]" ON)

# Build optimized code for following CPU (default i386)
#SET(CPU_TUNE               "i686")

# Floating point precision for formatting output variables
SET(CTPP_FLOAT_PRECISION   12)

# Escape and tempoprary buffer size
SET(CTPP_ESCAPE_BUFFER_LEN 1024)

# Max. template recursion depth
SET(CTPP_MAX_TEMPLATE_RECURSION_DEPTH 1024)

# ################################################################################
# Standard library
#
# OPTION(GETTEXT_SUPPORT     "Enable gettext support [default: ON]"               ON)
# OPTION(MD5_SUPPORT         "Enable md5 support [default: ON]"                   ON)
# OPTION(ICONV_SUPPORT       "Enable iconv support [default: ON]"                 ON)

SET(GETTEXT_SUPPORT 1)
SET(MD5_SUPPORT 1)
SET(ICONV_SUPPORT 1)

# ################################################################################
# Check Includes
#
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckLibraryExists)

CHECK_LIBRARY_EXISTS(m round    "" HAVE_ROUND_FN)
CHECK_LIBRARY_EXISTS(m roundl   "" HAVE_ROUNDL_FN)

CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/time.h  HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/uio.h   HAVE_SYS_UIO_H)

CHECK_INCLUDE_FILES(fcntl.h     HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(math.h      HAVE_MATH_H)
CHECK_INCLUDE_FILES(stdio.h     HAVE_STDIO_H)
CHECK_INCLUDE_FILES(stdlib.h    HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(string.h    HAVE_STRING_H)
CHECK_INCLUDE_FILES(strings.h   HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(time.h      HAVE_TIME_H)
CHECK_INCLUDE_FILES(unistd.h    HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(sysexits.h  HAVE_SYSEXITS_H)

CHECK_CXX_SOURCE_COMPILES("#include <map>
                           namespace std { }
                           using namespace std;
                           int main(void)
                           {
                               map<int, int> m;
                               m.insert(pair<int, int>(1, 2));
                               m[2] = 3;
                               return 0;
                           }
                          " STL_MAP_CHECK)
IF(NOT STL_MAP_CHECK)
    MESSAGE(ERROR "!!!! STL map does not work!")
ENDIF(NOT STL_MAP_CHECK)

CHECK_CXX_SOURCE_COMPILES("#include <vector>
                           namespace std { }
                           using namespace std;
                           int main(void)
                           {
                               vector<int> v;
                               v.push_back(1);
                               return 0;
                           }
                          " STL_VECTOR_CHECK)
IF(NOT STL_VECTOR_CHECK)
    MESSAGE(ERROR "!!!! STL vector does not work!")
ENDIF(NOT STL_VECTOR_CHECK)

CHECK_CXX_SOURCE_COMPILES("#include <string>
                           using namespace std;
                           int main(void)
                           {
                               string s;
                               s.assign(\"Hello, World!\");
                               return 0;
                           }
                          " STL_STRING_CHECK)

IF(NOT STL_STRING_CHECK)
    MESSAGE(ERROR "!!!! STL string does not work!")
ENDIF(NOT STL_STRING_CHECK)

CHECK_CXX_SOURCE_COMPILES("#include <stdexcept>
                           namespace std { }
                           using namespace std;
                           int main(void)
                           {
                               try
                               {
                                   throw logic_error(\"Ouch!\");
                               }
                               catch(exception &e)
                               {
                                   return 0;
                               }
                               return 0;
                           }
                          " STL_STDEXCEPT_CHECK)

IF(NOT STL_STDEXCEPT_CHECK)
    MESSAGE(ERROR "!!!! STL logic_error/exception does not work!")
ENDIF(NOT STL_STDEXCEPT_CHECK)

CHECK_CXX_SOURCE_COMPILES("#include <vector>
                           int main(void)
                           {
                               vector<int> v;
                               v.push_back(1);
                               return 0;
                           }
                          " STL_VECTOR_NO_STD_CHECK)

IF (STL_VECTOR_NO_STD_CHECK)
    MESSAGE(STATUS "#### INFO: Using STL classes without STD namespace")
    SET(NO_STL_STD_PREFIX "ON")
ELSE (STL_VECTOR_NO_STD_CHECK)
    MESSAGE(STATUS "#### INFO: Using STL classes with STD namespace")
ENDIF (STL_VECTOR_NO_STD_CHECK)

MESSAGE(STATUS "#### System name is: ${CMAKE_SYSTEM_NAME}")

IF("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
    MESSAGE(STATUS "#### Building for FreeBSD...")
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")

IF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

    SET(SKIP_ICONV_LIRARY   "ON")
    SET(SKIP_LIBINTL_LIRARY "ON")

    MESSAGE(STATUS "#### Building for Linux...")

    IF("${SKIP_RELINK_RPATH}" MATCHES "ON")
        SET(CMAKE_SKIP_RPATH  TRUE)

        MESSAGE(STATUS "#### Building for Linux...")
        # skip the full RPATH for the build tree
        SET(CMAKE_SKIP_BUILD_RPATH  TRUE)

        # when building, use the install RPATH already
        # (so it doesn't need to relink when installing)
        SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

        # the RPATH to be used when installing
        SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

        # add the automatically determined parts of the RPATH
        # which point to directories outside the build tree to the install RPATH
        SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    ENDIF("${SKIP_RELINK_RPATH}" MATCHES "ON")

ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

IF("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    SET(CMAKE_SKIP_RPATH FALSE)
    SET(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-headerpad_max_install_names")
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")

IF("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
    SET(SKIP_ICONV_LIRARY   "ON")
    MESSAGE(STATUS "#### Building for SunOS...")
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")

IF("${CMAKE_SYSTEM_NAME}" MATCHES "HP-UX")
    MESSAGE(STATUS "#### Building for HP-UX...")
ENDIF("${CMAKE_SYSTEM_NAME}" MATCHES "HP-UX")

IF(WIN32 OR MINGW)
    INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/libs/win32/include")
ENDIF(WIN32 OR MINGW)

IF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

SET(GETTEXT_SUPPORT "ON")

SET(MD5_WITHOUT_OPENSSL "")
#
# MD5 library support
#
IF (${MD5_SUPPORT} MATCHES "ON")
    IF (WIN32 OR MINGW)
        MESSAGE(STATUS "#### INFO: MD5 (Win32 CryptoAPI) hash function support enabled")
    ELSE (WIN32 OR MINGW)

        FIND_PATH(MD5_INCLUDE openssl/md5.h PATHS /opt/include
                                                  /usr/include
                                                  /usr/local/include
                  DOC "Path where the openssl header files can be found")

        IF (MD5_INCLUDE)
            INCLUDE_DIRECTORIES(${MD5_INCLUDE})

            FIND_LIBRARY(MD5_LIBRARY NAMES crypto PATHS /lib
                                                        /opt/lib
                                                        /usr/lib
                                                        /usr/local/lib
                         DOC "Path where the openssl libraries can be found")
            IF (MD5_LIBRARY)
                SET(PROJECT_LIBS "${PROJECT_LIBS} crypto")
            ELSE (MD5_LIBRARY)
                MESSAGE(STATUS "#### ERROR: Cannot find libcrypto.so, aborting")
            ENDIF (MD5_LIBRARY)

            SET(MD5_SUPPORT "ON")

            MESSAGE(STATUS "#### INFO: MD5 (openssl) hash function support enabled")

        ELSE (MD5_INCLUDE)
            # reset FIND_PATH
            SET(MD5_INCLUDE)
            FIND_PATH(MD5_INCLUDE md5.h PATHS /opt/include
                                              /usr/include
                                              /usr/local/include
                  DOC "Path where the openssl header files can be found")
            IF (MD5_INCLUDE)
                INCLUDE_DIRECTORIES(${MD5_INCLUDE})

                FIND_LIBRARY(MD5_LIBRARY NAMES md5 PATHS /lib
                                                         /opt/lib
                                                         /usr/lib
                                                         /usr/local/lib
                             DOC "Path where the md5 libraries can be found")
                IF (MD5_LIBRARY)
                    SET(PROJECT_LIBS "${PROJECT_LIBS} md5")
                    SET(MD5_SUPPORT         "ON")
                    SET(MD5_WITHOUT_OPENSSL "ON")
                    MESSAGE(STATUS "#### INFO: MD5 (libmd5) hash function support enabled")
                ELSE (MD5_LIBRARY)
                    MESSAGE(STATUS "#### ERROR: Cannot find md5.so, aborting")
                ENDIF (MD5_LIBRARY)
            ELSE (MD5_INCLUDE)
                SET(MD5_SUPPORT "")
                MESSAGE(STATUS "#### WARNING: MD5 hash function support disabled: md5.h not found")
            ENDIF (MD5_INCLUDE)

        ENDIF (MD5_INCLUDE)

    ENDIF (WIN32 OR MINGW)
ENDIF (${MD5_SUPPORT} MATCHES "ON")

SET(ICONV_SUPPORT "ON")

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(include/functions)

IF("${CMAKE_COMPILER_IS_GNUCXX}" MATCHES "1")

    SET(CMAKE_CXX_PARAMS      " --param large-function-growth=5000 --param inline-unit-growth=600 -finline-limit=2000 ")

    SET(CMAKE_CXX_WARN_FLAGS  " -Wall -Wno-long-long -Wno-inline -finline-functions ")

    IF(ENABLE_OPTIMIZATION MATCHES "ON")
        SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} ${CMAKE_CXX_WARN_FLAGS} ${CMAKE_CXX_PARAMS} -O3 ")

        IF (NOT "${CPU_TUNE}" STREQUAL "")
            SET(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -mtune=${CPU_TUNE} ")
        ENDIF (NOT "${CPU_TUNE}" STREQUAL "")

    ELSE(ENABLE_OPTIMIZATION MATCHES "OFF")
        SET(CMAKE_CXX_FLAGS "${CXX_FLAGS} ${CMAKE_CXX_WARN_FLAGS} ${CMAKE_CXX_PARAMS} ")
    ENDIF(ENABLE_OPTIMIZATION MATCHES "ON")

    IF(DEBUG_MODE MATCHES "ON")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
    ENDIF(DEBUG_MODE MATCHES "ON")

ENDIF("${CMAKE_COMPILER_IS_GNUCXX}" MATCHES "1")

IF(MSVC)
    IF(ENABLE_OPTIMIZATION MATCHES "ON")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
    ENDIF(ENABLE_OPTIMIZATION MATCHES "ON")
ENDIF(MSVC)

IF(WIN32 OR MINGW)
    ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
    ADD_DEFINITIONS(-DWIN32)
ENDIF(WIN32 OR MINGW)

IF(ENABLE_OPTIMIZATION MATCHES "ON")
    MESSAGE(STATUS "#### INFO: Optimized build mode enabled")
ELSE(ENABLE_OPTIMIZATION MATCHES "OFF")
    MESSAGE(STATUS "#### INFO: Optimized build mode disabled")
ENDIF(ENABLE_OPTIMIZATION MATCHES "ON")

IF(DEBUG_MODE MATCHES "ON")
    MESSAGE(STATUS "#### ATTENTION: Debug mode enabled. Use this option only for testing!")
ENDIF(DEBUG_MODE MATCHES "ON")

# ################################################################################
# Sources
#
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/CTPP2SysHeaders.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/include/CTPP2SysHeaders.h)
#
SET(LIBSRCS
            src/CDT.cpp
            src/CDTSortRoutines.cpp

            src/CTPP2BitIndex.cpp
            src/CTPP2Compiler.cpp
            src/CTPP2DTOA.cpp
            src/CTPP2Exception.cpp
            src/CTPP2Error.cpp
            src/CTPP2FileOutputCollector.cpp
            src/CTPP2FileSourceLoader.cpp
            src/CTPP2FileLogger.cpp
            src/CTPP2HashTable.cpp
            src/CTPP2JSONParser.cpp
            src/CTPP2JSONFileParser.cpp
            src/CTPP2Logger.cpp
            src/CTPP2Parser.cpp
            src/CTPP2ParserException.cpp
            src/CTPP2SimpleCompiler.cpp
            src/CTPP2SimpleVM.cpp
            src/CTPP2Sprintf.cpp
            src/CTPP2StringBuffer.cpp
            src/CTPP2StaticData.cpp
            src/CTPP2StaticText.cpp
            src/CTPP2StreamOutputCollector.cpp
            src/CTPP2StringOutputCollector.cpp
            src/CTPP2StringIconvOutputCollector.cpp
            src/CTPP2SyscallFactory.cpp
            src/CTPP2Util.cpp
            src/CTPP2VM.cpp
            src/CTPP2VMArgStack.cpp
            src/CTPP2VMCodeStack.cpp
            src/CTPP2VMDebugInfo.cpp
            src/CTPP2VMDumper.cpp
            src/CTPP2VMException.cpp
            src/CTPP2VMExecutable.cpp
            src/CTPP2VMFileLoader.cpp
            src/CTPP2VMMemoryCore.cpp
            src/CTPP2VMOpcodeCollector.cpp
            src/CTPP2VMSTDLib.cpp
            src/CTPP2VMSyscall.cpp

            src/functions/FnArrayElement.cpp
            src/functions/FnAvg.cpp
            src/functions/FnBase64Decode.cpp
            src/functions/FnBase64Encode.cpp
            src/functions/FnCast.cpp
            src/functions/FnConcat.cpp
            src/functions/FnDateFormat.cpp
            src/functions/FnDefault.cpp
            src/functions/FnDefined.cpp
            src/functions/FnEmitter.cpp
            src/functions/FnError.cpp
            src/functions/FnFormParam.cpp
            src/functions/FnGetText.cpp
            src/functions/FnGetText_.cpp
            src/functions/FnGetType.cpp
            src/functions/FnHashElement.cpp
            src/functions/FnHMACMD5.cpp
            src/functions/FnHTMLEscape.cpp
            src/functions/FnHrefParam.cpp
            src/functions/FnHostname.cpp
            src/functions/FnIconv.cpp
            src/functions/FnInSet.cpp
            src/functions/FnJSEscape.cpp
            src/functions/FnJSON.cpp
            src/functions/FnListElement.cpp
            src/functions/FnLog.cpp
            src/functions/FnMBSize.cpp
            src/functions/FnMBSubstring.cpp
            src/functions/FnMBTruncate.cpp
            src/functions/FnMD5.cpp
            src/functions/FnMax.cpp
            src/functions/FnMin.cpp
            src/functions/FnNumFormat.cpp
            src/functions/FnNumeral.cpp
            src/functions/FnObjDump.cpp
            src/functions/FnRandom.cpp
            src/functions/FnSize.cpp
            src/functions/FnSprintf.cpp
            src/functions/FnSubstring.cpp
            src/functions/FnTruncate.cpp
            src/functions/FnURIEscape.cpp
            src/functions/FnURLEscape.cpp
            src/functions/FnVersion.cpp
            src/functions/FnWMLEscape.cpp
            src/functions/FnXMLEscape.cpp
           )

# ################################################################################
# Lib binary
#
ADD_LIBRARY(ctpp2-static STATIC ${LIBSRCS})
SET_TARGET_PROPERTIES(ctpp2-static PROPERTIES OUTPUT_NAME ctpp2-st)
SET_TARGET_PROPERTIES(ctpp2-static PROPERTIES LINKER_LANGUAGE CXX)

# End.
