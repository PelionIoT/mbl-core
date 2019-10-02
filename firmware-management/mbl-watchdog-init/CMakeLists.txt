cmake_minimum_required(VERSION 3.5)

project(mblWatchdogInit)

set(MBL_WATCHDOG_INIT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mbl-watchdog-init.c")

add_executable(mbl-watchdog-init ${MBL_WATCHDOG_INIT_SRC})

target_compile_options(mbl-watchdog-init 
    PUBLIC -Wall -Wpedantic -Werror
)
