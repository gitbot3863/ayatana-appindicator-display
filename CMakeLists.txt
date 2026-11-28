cmake_minimum_required (VERSION 3.20)
project (ayatana-appindicator-display VERSION 2026.11.24 LANGUAGES C)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET (CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_compile_options (-Wall)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options (-Wextra -Wpedantic -Werror)
endif ()

include (GNUInstallDirs)
find_package (PkgConfig REQUIRED)
pkg_check_modules (PROJECT REQUIRED IMPORTED_TARGET GLOBAL gio-unix-2.0 glib-2.0 libgeoclue-2.0 ayatana-appindicator-glib libayatana-common)
add_subdirectory (src)
add_subdirectory (data)
add_subdirectory (po)
