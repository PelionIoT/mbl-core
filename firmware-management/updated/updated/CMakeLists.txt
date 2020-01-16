# Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

if(${RUN_CODE_CHECKS})
    find_package(spdlog REQUIRED)
endif()

set(UPDATED_SRC
    source/init.cpp
    source/updated.cpp
    source/UpdateCoordinator.cpp

    source/rpc/Server.cpp
    source/rpc/ServiceImpl.cpp

    source/logging/logger.cpp
)

add_executable(updated ${UPDATED_SRC})
target_link_libraries(updated common_compile_options)
target_link_libraries(updated common_compile_warnings)
target_link_libraries(updated updated-rpc)
target_link_libraries(updated systemd)

if(${RUN_CODE_CHECKS})
    target_link_libraries(updated spdlog::spdog)
endif()

install(TARGETS updated DESTINATION ${CMAKE_INSTALL_BINDIR})
