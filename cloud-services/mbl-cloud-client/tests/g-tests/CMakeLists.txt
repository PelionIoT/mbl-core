# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required (VERSION 3.9)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

project(mblCcrbTests)

add_definitions(-DMBED_CLOUD_CLIENT_ENDPOINT_TYPE="default")
add_definitions(-DMBED_CLOUD_CLIENT_LIFETIME=600)
add_definitions(-DSN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE=512)
add_definitions(-DMBED_CLOUD_CLIENT_TRANSPORT_MODE_TCP)


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


include_directories(${CCRB_INCLUDE_DIR})
include_directories(${CCRB_INCLUDE_DIR}/source)

include_directories(${MBED_CLOUD_CLIENT_INC_DIR})

include_directories(${MBED_CLOUD_CLIENT_DIR})
include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client)
#include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client/source)

include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c/nsdl-c)

include_directories(${MBED_CLOUD_CLIENT_DIR}/nanostack-libservice/mbed-client-libservice)

include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap/mbed-coap)

include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal)


include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)

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


target_link_libraries(gtest-mbl-cloud-client libmblCloudClient.a)
target_link_libraries(gtest-mbl-cloud-client libmbedCloudClient.a)
target_link_libraries(gtest-mbl-cloud-client libmblCloudClient.a)

target_link_libraries(gtest-mbl-cloud-client factory-configurator-client)
target_link_libraries(gtest-mbl-cloud-client pal)
target_link_libraries(gtest-mbl-cloud-client mbedcrypto)
target_link_libraries(gtest-mbl-cloud-client mbedtls)
target_link_libraries(gtest-mbl-cloud-client mbedx509)

target_link_libraries(gtest-mbl-cloud-client mbedTrace)
target_link_libraries(gtest-mbl-cloud-client ${JSONCPP_LIBRARIES})

# mbedCloudClient seems to be co-dependent with mbedTrace (at least when
# MBED_CONF_MBED_TRACE_FEA_IPV6 == 1), hence the second mention here
target_link_libraries(gtest-mbl-cloud-client mbedCloudClient)

target_link_libraries(gtest-mbl-cloud-client libmblCloudClient.a)

target_link_libraries(gtest-mbl-cloud-client libgtest_main.a)
target_link_libraries(gtest-mbl-cloud-client libgtest.a)

target_link_libraries(gtest-mbl-cloud-client pthread)
target_link_libraries(gtest-mbl-cloud-client rt)
# link with systemd to get D-Bus libs
target_link_libraries(gtest-mbl-cloud-client systemd)

