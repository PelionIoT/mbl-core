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

#include_directories(${INCLUDE_DIRECTORIES})

include_directories(${CCRB_INCLUDE_DIR1})
include_directories(${CCRB_INCLUDE_DIR2})

include_directories(${MBED_CLOUD_CLIENT_DIR})
include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client)
#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client/source)

include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap)

#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client/mbed-client)
#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client/mbed-coap)
#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client/mbed-coap/mbed-coap)




include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)

 
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "@@@DIRS: ")
foreach(dir ${dirs})
  message(STATUS "@@@DIR='${dir}'")
endforeach()



# Create the Cloud Client executable
#FILE(GLOB MBL_CLOUD_CLIENT_SRC
#    "${CMAKE_CURRENT_SOURCE_DIR}/../../*.c"
#    "${CMAKE_CURRENT_SOURCE_DIR}/../../*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/../../source/*.c"
#    "${CMAKE_CURRENT_SOURCE_DIR}/../../source/*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/../../source/cloud-connect-resource-broker/*.cpp"
#)

# Create static library "mblCloudClient"
#add_library(mblCloudClient STATIC "${MBL_CLOUD_CLIENT_SRC}")


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
target_link_libraries(gtest-mbl-cloud-client libmblCloudClient.a)
target_link_libraries(gtest-mbl-cloud-client libmbedCloudClient.a)
target_link_libraries(gtest-mbl-cloud-client systemd)
target_link_libraries(gtest-mbl-cloud-client pthread)

