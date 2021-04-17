cmake_minimum_required(VERSION 3.0)
project(xms C)

set(CMAKE_BUILD_TYPE Debug)
set(BUILD_SHARED_LIBS OFF)

if(CMAKE_FIND_ROOT_PATH)
    set(CMAKE_SYSROOT "${CMAKE_FIND_ROOT_PATH}")

    set(ENV{PKG_CONFIG_DIR} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/lib/pkgconfig:${CMAKE_SYSROOT}/share/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(MPD QUIET libmpdclient)
if(NOT MPD_FOUND)
	message(ERROR "MPD client libraries are not found! Aborting...")
endif()

add_library(xms SHARED 
	"src/xms_song.c")

add_executable(xms_example
	"src/main.c")

add_definitions(${MPD_CFLAGS_OTHER})

target_link_libraries(xms ${MPD_LIBRARIES} m)
target_link_libraries(xms_example xms)

