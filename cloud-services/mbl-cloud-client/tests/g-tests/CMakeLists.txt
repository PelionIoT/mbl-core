# Copyright (c) 2019 ARM Ltd.
#
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 3.9)
SET(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

project(mblCcrbTests)

# Compile CCRB gtests using c++14 standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_compile_options(-Werror)
add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wformat=2)
add_compile_options(-Wunused-parameter)
add_compile_options(-Wfloat-equal)
add_compile_options(-Wshadow)
add_compile_options(-Wcast-qual)
add_compile_options(-Wcast-align)
add_compile_options(-Wwrite-strings)
add_compile_options(-Wconversion)
add_compile_options(-Wlogical-op)


include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${SYSTEMD_INCLUDE_DIRS})
include_directories(${INCLUDE_DIRS})


FILE(GLOB MBL_CCRB_TESTS_SRC
    "${CMAKE_SOURCE_DIR}/*.cpp"
)


add_executable(run-gtests ${MBL_CCRB_TESTS_SRC})

set_target_properties (run-gtests PROPERTIES
   CXX_STANDARD 14
   CXX_STANDARD_REQUIRED TRUE
   CXX_EXTENSIONS FALSE
)


target_link_libraries(run-gtests "libgtest_main.a")
target_link_libraries(run-gtests "libgtest.a")
target_link_libraries(run-gtests "libmbedCcrb.a")
target_link_libraries(run-gtests pthread)

