cmake_minimum_required(VERSION 3.5)

project(swupdate-handlers)

add_library(swupdate-handlers SHARED rootfs-handler.c arm-handler-common.c)

target_include_directories(swupdate-handlers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(swupdate-handlers
    PUBLIC
    -Wall
    -Wextra
    -Werror
    -Wformat=2
    -Wunused-parameter
    -Wfloat-equal
    -Wshadow
    -Wcast-qual
    -Wcast-align
    -Wwrite-strings
    -Wconversion
    -Wlogical-op
)

set(BOOTFLAGS_DIR "/var/bootflags" CACHE PATH "Path to directory containing boot flag files")
set(TMP_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store small files")
set(UPDATE_PAYLOAD_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store update payload files")
set(PAYLOAD_TMP_DIR "${UPDATE_PAYLOAD_DIR}/arm_update_activate" CACHE PATH "Path to directory used to temporarily store payload files")
set(LOG_DIR "/var/log" CACHE FILEPATH "Path to directory in which to write log files")
set(FACTORY_CONFIG_DIR "/config/factory" CACHE STRING "Factory config partition mount point.")
set(PART_INFO_DIR "${FACTORY_CONFIG_DIR}/part-info" CACHE PATH "Path to the directory containing information about the partition layout")
# Replace placeholder variables with our cache variables defined above.
configure_file("arm-handler-common.h.in" "arm-handler-common.h" @ONLY)

install(TARGETS swupdate-handlers DESTINATION lib)
