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
#cmake_minimum_required (VERSION 3.9)
SET(CMAKE_SYSTEM_NAME Generic)

project(mblCloudClient)

# Compile mbl-cloud-client using c++14 standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

ADDSUBDIRS()

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

# TODO: ensure there are secure versions of this in mbed-client-pal for the
# platforms we need to support and use those instead
# Currently the compilation flag PAL_USE_HW_ROT is 0, which means that mbedPalInsecureROT library is not supported on HW.
# If this will be changed a proper implementation will need to be used.

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


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/cloud-connect-resource-broker)

SET(MBED_CLOUD_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbed-cloud-client)

include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)


# Create the Cloud Client main executable
FILE(GLOB MBL_CLOUD_CLIENT_MAIN
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/g-tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    #"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/cloud-connect-resource-broker/*.cpp"
)
add_executable(mbl-cloud-client_gtest "${MBL_CLOUD_CLIENT_MAIN}")


# Create the Cloud Client static library
#FILE(GLOB MBL_CLOUD_CLIENT_SRC
#    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
#    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
#    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
#    "${CMAKE_CURRENT_SOURCE_DIR}/source/cloud-connect-resource-broker/*.cpp"
#)
# Create static library "mblCloudClient"
#add_library(mblCloudClient STATIC "${MBL_CLOUD_CLIENT_SRC}")


# Order is important here: dependencies must follow dependents

target_link_libraries(mbl-cloud-client_gtest libgtest_main.a)
target_link_libraries(mbl-cloud-client_gtest libgtest.a)

#target_link_libraries(mbl-cloud-client_gtest mblCloudClient)
target_link_libraries(mbl-cloud-client_gtest mbedCloudClient)
target_link_libraries(mbl-cloud-client_gtest factory-configurator-client)
target_link_libraries(mbl-cloud-client_gtest pal)
target_link_libraries(mbl-cloud-client_gtest mbedcrypto)
target_link_libraries(mbl-cloud-client_gtest mbedtls)
target_link_libraries(mbl-cloud-client_gtest mbedx509)

target_link_libraries(mbl-cloud-client_gtest mbedTrace)
target_link_libraries(mbl-cloud-client_gtest ${JSONCPP_LIBRARIES})

# mbedCloudClient seems to be co-dependent with mbedTrace (at least when
# MBED_CONF_MBED_TRACE_FEA_IPV6 == 1), hence the second mention here
target_link_libraries(mbl-cloud-client_gtest mbedCloudClient)

target_link_libraries(mbl-cloud-client_gtest pthread)
target_link_libraries(mbl-cloud-client_gtest rt)
# link with systemd to get D-Bus libs
target_link_libraries(mbl-cloud-client_gtest systemd)

