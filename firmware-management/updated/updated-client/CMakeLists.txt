# Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

set(UPDATED_GET_STATUS_SRC
    source/rpc/Client.cpp
    source/updated-get-status.cpp
)

set(UPDATED_START_UPDATE_SRC
    source/rpc/Client.cpp
    source/updated-start-update.cpp
)

add_executable(updated-get-status ${UPDATED_GET_STATUS_SRC})

add_executable(updated-start-update ${UPDATED_START_UPDATE_SRC})

target_link_libraries(updated-start-update common_compile_options)
target_link_libraries(updated-start-update common_compile_warnings)
target_link_libraries(updated-start-update updated-rpc)

target_link_libraries(updated-get-status common_compile_options)
target_link_libraries(updated-get-status common_compile_warnings)
target_link_libraries(updated-get-status updated-rpc)

install(TARGETS updated-get-status updated-start-update DESTINATION ${CMAKE_INSTALL_BINDIR})
