# Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.5)

project(mblWatchdogInit)

set(MBL_WATCHDOG_INIT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mbl-watchdog-init.c")

add_executable(mbl-watchdog-init ${MBL_WATCHDOG_INIT_SRC})

target_compile_options(mbl-watchdog-init
    PUBLIC -Wall -Wextra -Wpedantic -Werror -Wshadow -Wcast-align -Wwrite-strings -Wunreachable-code
)

install(TARGETS mbl-watchdog-init DESTINATION bin)
