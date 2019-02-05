# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0


include(${EXTERNAL_DEFINE_FILE})

cmake_minimum_required (VERSION 3.9)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

project(mblCloudClientGtest)

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

include_directories(${MBL_CLOUD_CLIENT_INCLUDE_DIR})
include_directories(${MBL_CLOUD_CLIENT_INCLUDE_DIR}/source)
include_directories(${INC_DIR})
include_directories(${MBED_CLOUD_CLIENT_DIR})
include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client)
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
include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub)
include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub/modules/common)

FILE(GLOB MBL_CLOUD_CLIENT_GTESTS_SRC
    "${CMAKE_SOURCE_DIR}/*.cpp"
)
add_executable(mbl-cloud-client-gtest ${MBL_CLOUD_CLIENT_GTESTS_SRC})


set_target_properties (mbl-cloud-client-gtest PROPERTIES
   CXX_STANDARD 14
   CXX_STANDARD_REQUIRED TRUE
   CXX_EXTENSIONS FALSE
)

target_link_libraries(mbl-cloud-client-gtest libmblCloudClient.a)
target_link_libraries(mbl-cloud-client-gtest libmbedCloudClient.a)
# Cyclic dependency between mblCloudClient and mbedCloudClient
target_link_libraries(mbl-cloud-client-gtest libmblCloudClient.a)

target_link_libraries(mbl-cloud-client-gtest factory-configurator-client)
target_link_libraries(mbl-cloud-client-gtest pal)
target_link_libraries(mbl-cloud-client-gtest mbedcrypto)
target_link_libraries(mbl-cloud-client-gtest mbedtls)
target_link_libraries(mbl-cloud-client-gtest mbedx509)

target_link_libraries(mbl-cloud-client-gtest mbedTrace)
target_link_libraries(mbl-cloud-client-gtest ${JSONCPP_LIBRARIES})

# mbedCloudClient seems to be co-dependent with mbedTrace (at least when
# MBED_CONF_MBED_TRACE_FEA_IPV6 == 1), hence the second mention here
target_link_libraries(mbl-cloud-client-gtest mbedCloudClient)
# Cyclic dependency between mblCloudClient and mbedCloudClient
target_link_libraries(mbl-cloud-client-gtest libmblCloudClient.a)

target_link_libraries(mbl-cloud-client-gtest libgtest_main.a)
target_link_libraries(mbl-cloud-client-gtest libgtest.a)

target_link_libraries(mbl-cloud-client-gtest pthread)
target_link_libraries(mbl-cloud-client-gtest rt)
# link with systemd to get sd-bus and sd-event support
target_link_libraries(mbl-cloud-client-gtest systemd)
