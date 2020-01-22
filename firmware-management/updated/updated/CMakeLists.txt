# Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

set(UPDATED_SRC
    source/UpdateCoordinator.cpp
    source/updated.cpp

    source/daemon/init.cpp
    source/fileutils/LockFile.cpp
    source/logging/logger.cpp
    source/rpc/Server.cpp
    source/rpc/ServiceImpl.cpp
    source/signal/handlers.cpp
)

add_executable(updated ${UPDATED_SRC})
target_link_libraries(updated common_compile_options)
target_link_libraries(updated common_compile_warnings)
target_link_libraries(updated updated-rpc)
target_link_libraries(updated systemd)

install(TARGETS updated DESTINATION ${CMAKE_INSTALL_BINDIR})
