set (APP_NAME opentube)
project (${APP_NAME})
cmake_minimum_required (VERSION 2.6)
add_subdirectory (libs/ctpp2 libs/ctpp2)

set (GETTEXT_DOMAIN "${APP_NAME}")
set (LOCALE_DIR "locale")

add_subdirectory (po)
add_subdirectory (src objs)
