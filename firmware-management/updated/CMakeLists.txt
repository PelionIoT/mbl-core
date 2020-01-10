# Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


project(UpdateD)

cmake_minimum_required(VERSION 3.5)

option(RUN_CODE_CHECKS OFF)

# INTERFACE library for compile options that are not for enabling warnings.
# To maintain ABI compatibility between libs/executables, every C++ target
# should do "target_link_libraries(target common_compile_options).
add_library(common_compile_options INTERFACE)
target_compile_options(common_compile_options INTERFACE
    -std=c++17
)

# INTERFACE library for compile options that are for enabling warnings/errors.
# Options that affect ABI compatibility should NOT go here.
add_library(common_compile_warnings INTERFACE)
target_compile_options(common_compile_warnings INTERFACE
    -Wall
    -Werror
    -Wextra
    -Wformat=2
    -Wunused-parameter
    -Wfloat-equal
    -Wshadow
    -Wcast-qual
    -Wcast-align
    -Wwrite-strings
    -Wconversion
    "$<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>")

add_subdirectory("updated-rpc")
add_subdirectory("updated")
add_subdirectory("updated-client")
