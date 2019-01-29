# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required (VERSION 3.9)
SET(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

project(mblCcrbTests)


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


include_directories(${SYSTEMD_INCLUDE_DIRS})
include_directories(${MBEDCCRB_INCLUDE_DIRS})
include_directories(${INCLUDE_DIRS})

include_directories(${INCLUDE_DIRECTORIES})

#include_directories("/home/yelgor01/mbl/mbl-max/mbl-master_PQ331/build-mbl/tmp-mbl-glibc/work/armv7vet2hf-neon-oe-linux-gnueabi/ccrb-test/1.0-r0/git/cloud-services/mbl-cloud-client/source/")
#include_directories("/home/yelgor01/mbl/mbl-max/mbl-master_PQ331/build-mbl/tmp-mbl-glibc/work/armv7vet2hf-neon-oe-linux-gnueabi/ccrb-test/1.0-r0/git/cloud-services/mbl-cloud-client/source/cloud-connect-resource-broker/")

message(STATUS "@@@CMAKE_CURRENT_SOURCE_DIR='${CMAKE_CURRENT_SOURCE_DIR}'")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../source)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../source/cloud-connect-resource-broker)

include_directories(${CCRB_INCLUDE_DIRS})
message(STATUS "@@@CCRB_INCLUDE_DIRS='${CCRB_INCLUDE_DIRS}'")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "@@@DIRS: ")
foreach(dir ${dirs})
  message(STATUS "@@@DIR='${dir}'")
endforeach()

FILE(GLOB MBL_CCRB_TESTS_SRC
    "${CMAKE_SOURCE_DIR}/*.cpp"
)


add_executable(gtest-mbl-cloud-client ${MBL_CCRB_TESTS_SRC})

set_target_properties (gtest-mbl-cloud-client PROPERTIES
   CXX_STANDARD 14
   CXX_STANDARD_REQUIRED TRUE
   CXX_EXTENSIONS FALSE
)


target_link_libraries(gtest-mbl-cloud-client libgtest_main.a)
target_link_libraries(gtest-mbl-cloud-client libgtest.a)
target_link_libraries(gtest-mbl-cloud-client libmbedCcrb.a)
target_link_libraries(gtest-mbl-cloud-client systemd)
target_link_libraries(gtest-mbl-cloud-client pthread)

