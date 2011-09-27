set (APP_NAME opentube)
project (${APP_NAME} CXX C)
cmake_minimum_required (VERSION 2.6)

add_subdirectory (libs/jemalloc libs/jemalloc)

set (GETTEXT_DOMAIN "${APP_NAME}")
set (LOCALE_DIR "locale")

add_subdirectory (po locale)
add_subdirectory (src objs)
