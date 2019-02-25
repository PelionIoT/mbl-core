# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required (VERSION 2.8)

project(pelionProvisioningUtil)

add_compile_options(-Werror)
add_compile_options(-Wall)
add_compile_options(-Wwrite-strings)
add_compile_options(-Wconversion)
add_compile_options(-Wlogical-op)

FILE(GLOB PELION_PROVISIONING_UTIL_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
add_executable(pelion-provisioning-util ${PELION_PROVISIONING_UTIL_SRC})

target_link_libraries(pelion-provisioning-util mbedCloudClient)
target_link_libraries(pelion-provisioning-util factory-configurator-client)
target_link_libraries(pelion-provisioning-util certificate-enrollment-client)
target_link_libraries(pelion-provisioning-util mbedTrace)
target_link_libraries(pelion-provisioning-util esfs)

