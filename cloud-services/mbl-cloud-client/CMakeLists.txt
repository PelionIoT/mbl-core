# Copyright (c) 2017 ARM Ltd.
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

cmake_minimum_required (VERSION 2.8)
SET(CMAKE_SYSTEM_NAME Generic)

project(mblCloudClient)

ADDSUBDIRS()

# TODO: ensure there are secure versions of this in mbed-client-pal for the
# platforms we need to support and use those instead
add_library(mbedPalInsecureROT "${CMAKE_CURRENT_SOURCE_DIR}/mbed-cloud-client/mbed-client-pal/Examples/PlatformBSP/pal_insecure_ROT.c")

# Add these options after all non-mbl-cloud-client bits so they only apply to
# mbl-cloud-client code
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

SET(MBED_CLOUD_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbed-cloud-client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mbed-cloud-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap/mbed-coap)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/fcc-output-info-handler/fcc-output-info-handler)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/key-config-manager/key-config-manager)
include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub)
include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub/modules/common)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)
include_directories(${MBED_CLOUD_CLIENT_DIR}/nanostack-libservice/mbed-client-libservice)

FILE(GLOB MBL_CLOUD_CLIENT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)


add_executable(mbl-cloud-client "${MBL_CLOUD_CLIENT_SRC}")

# Order is important here: dependencies must follow dependents
target_link_libraries(mbl-cloud-client mbedCloudClient)
target_link_libraries(mbl-cloud-client factory-configurator-client)
target_link_libraries(mbl-cloud-client pal)
target_link_libraries(mbl-cloud-client mbedcrypto)
target_link_libraries(mbl-cloud-client mbedtls)
target_link_libraries(mbl-cloud-client mbedx509)
target_link_libraries(mbl-cloud-client mbedTrace)

# mbedCloudClient seems to be co-dependent with mbedTrace (at least when
# MBED_CONF_MBED_TRACE_FEA_IPV6 == 1), hence the second mention here
target_link_libraries(mbl-cloud-client mbedCloudClient)

target_link_libraries(mbl-cloud-client mbedPalInsecureROT)
target_link_libraries(mbl-cloud-client pthread)
target_link_libraries(mbl-cloud-client rt)
