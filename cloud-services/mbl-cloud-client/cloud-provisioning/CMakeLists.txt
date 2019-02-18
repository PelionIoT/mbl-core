# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

# include(${EXTERNAL_DEFINE_FILE})

cmake_minimum_required (VERSION 2.8)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")


project(pelionProvisioningUtil)
set(MBED_CLOUD_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../mbed-cloud-client)

find_package(PkgConfig REQUIRED)

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

include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-cloud-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c/nsdl-c)
include_directories(${MBED_CLOUD_CLIENT_DIR}/nanostack-libservice/mbed-client-libservice)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap/mbed-coap)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/key-config-manager)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)

FILE(GLOB PELION_PROVISIONING_UTIL_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_executable(pelion-provisioning-util ${PELION_PROVISIONING_UTIL_SRC})

target_link_libraries(pelion-provisioning-util mbedCloudClient)
target_link_libraries(pelion-provisioning-util factory-configurator-client)
target_link_libraries(pelion-provisioning-util certificate-enrollment-client)
target_link_libraries(pelion-provisioning-util pal)
target_link_libraries(pelion-provisioning-util mbedcrypto)
target_link_libraries(pelion-provisioning-util mbedtls)
target_link_libraries(pelion-provisioning-util mbedx509)
target_link_libraries(pelion-provisioning-util mbedTrace)
target_link_libraries(pelion-provisioning-util esfs)
target_link_libraries(pelion-provisioning-util nshalpal)
target_link_libraries(pelion-provisioning-util sotp)
target_link_libraries(pelion-provisioning-util nanostacklibservice)
target_link_libraries(pelion-provisioning-util mbedCloudClient)

target_link_libraries(pelion-provisioning-util pthread)
target_link_libraries(pelion-provisioning-util rt)
