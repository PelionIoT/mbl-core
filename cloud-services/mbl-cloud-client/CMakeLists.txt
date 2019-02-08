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

# generate compilation comands database that will be used by clang-tidy code checker
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

SET(MBED_CLOUD_CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbed-cloud-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/mbed-trace-helper)
include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client)
include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-trace)

FILE(GLOB MBL_CLOUD_CLIENT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/cloud-connect-resource-broker/*.cpp"
)


option(CODE_CHECK_MODE "Code check cmake mode" OFF)
if(CODE_CHECK_MODE)
  # include directories required for clang-tidy code checker
  include_directories(${MBED_CLOUD_CLIENT_DIR})
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client-c)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client/mbed-client)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/source)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-coap/mbed-coap)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/nanostack-libservice/mbed-client-libservice)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/update-client-hub/modules/common)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal/Source/PAL-Impl/Services-API)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal/Configs/pal_config/Linux)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/fcc-output-info-handler/fcc-output-info-handler)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal/Configs/pal_config/Linux/TARGET_x86_x64)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/key-config-manager/key-config-manager)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/factory-configurator-client/factory-configurator-client/factory-configurator-client)
  include_directories(${MBED_CLOUD_CLIENT_DIR}/mbed-client-pal/Source/Port/Platform-API)

  # Find path and verify installation of clang and clang-tidy
  find_program(CLANG "clang") 
  if(NOT CLANG)
    message(FATAL_ERROR "clang not installed! Abort.")
  endif()

  find_program(CLANG_TIDY "clang-tidy") 
  if(NOT CLANG_TIDY)
    message(FATAL_ERROR "clang-tidy not installed! Abort.")
  endif()

  # List of possible values for clang-tidy -checks parameter is listed on page:
  # http://releases.llvm.org/7.0.0/tools/clang/tools/extra/docs/clang-tidy/index.html
  # Important additonal values are: 
  # *                  Enable all possible checks
  # -*                 Remove all checks\
  set(CLANG_TIDY_CHECKS "-*,bugprone-*,cert-*,cppcoreguidelines-*,clang-analyzer-*,modernize-*,performance-*,readability-*")

  add_custom_target(
      clang-tidy
      COMMAND ${CLANG_TIDY}
      -p=${EXECUTABLE_OUTPUT_PATH}/../
      -export-fixes=${EXECUTABLE_OUTPUT_PATH}/../clang-tidy-suggested-fixes.txt 
      -checks=${CLANG_TIDY_CHECKS}
      ${MBL_CLOUD_CLIENT_SRC}
  )

  # Find path and verify installation of clang-format
  find_program(CLANG_FORMAT "clang-format") 
  if(NOT CLANG_FORMAT)
    message(FATAL_ERROR "clang-format not installed! Abort.")
  endif()
  
  add_custom_target(
      clang-format
      COMMAND ${CLANG_FORMAT}
      -i
      -style=file
      ${MBL_CLOUD_CLIENT_SRC}
  )

else()
  # -Wlogical-op GCC compilation flag causes error when clang-tidy uses compilation commands list,
  #              so we add this option only clang-tid is not used 
  add_compile_options(-Wlogical-op)

endif(CODE_CHECK_MODE)
unset(CODE_CHECK_MODE CACHE)


add_executable(mbl-cloud-client "${MBL_CLOUD_CLIENT_SRC}")

# Order is important here: dependencies must follow dependents
target_link_libraries(mbl-cloud-client mbedCloudClient)
target_link_libraries(mbl-cloud-client factory-configurator-client)
target_link_libraries(mbl-cloud-client pal)
target_link_libraries(mbl-cloud-client mbedcrypto)
target_link_libraries(mbl-cloud-client mbedtls)
target_link_libraries(mbl-cloud-client mbedx509)
target_link_libraries(mbl-cloud-client mbedTrace)
target_link_libraries(mbl-cloud-client ${JSONCPP_LIBRARIES})

# mbedCloudClient seems to be co-dependent with mbedTrace (at least when
# MBED_CONF_MBED_TRACE_FEA_IPV6 == 1), hence the second mention here
target_link_libraries(mbl-cloud-client mbedCloudClient)

target_link_libraries(mbl-cloud-client pthread)
target_link_libraries(mbl-cloud-client rt)

# link with systemd to get D-Bus libs
target_link_libraries(mbl-cloud-client systemd)

