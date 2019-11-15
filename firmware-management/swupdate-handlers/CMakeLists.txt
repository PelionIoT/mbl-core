cmake_minimum_required(VERSION 3.5)

project(swupdate-handlers)

add_library(swupdate-handlers OBJECT rootfs-handler.c arm-handler-common.c)

target_include_directories(swupdate-handlers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(swupdate-handlers PUBLIC -Wall -Wextra)

set(BOOTFLAGS_DIR "/var/bootflags" CACHE PATH "Path to directory containing boot flag files")
set(TMP_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store small files")
set(UPDATE_PAYLOAD_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store update payload files")
set(PAYLOAD_TMP_DIR "${UPDATE_PAYLOAD_DIR}/arm_update_activate" CACHE PATH "Path to directory used to temporarily store payload files")
set(LOG_DIR "/var/log" CACHE FILEPATH "Path to directory in which to write log files")
set(ROOTFS_TYPE "ext4" CACHE STRING "Type of file system used for root file systems")
set(FACTORY_CONFIG_PARTITION "/config/factory" CACHE STRING "Factory config partition mount point.")

# Replace placeholder variables with our cache variables defined above.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/arm-handler-common.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/arm-handler-common.h" @ONLY)
