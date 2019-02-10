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

cmake_minimum_required (VERSION 2.8)
project(code-check)

if(NOT CODE_CHECK_SRC)
  message(FATAL_ERROR "Source list for code checkers is empty! Abort.")
endif()

message("Run code check on file list \n ${CODE_CHECK_SRC}")

# generate compilation comands database that will be used by clang-tidy code checker
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
set(CLANG_TIDY_CHECKS "-*,bugprone-*,cert-*,cppcoreguidelines-*,clang-analyzer-*,modernize-*,performance-*,readability-*,-cppcoreguidelines-pro-type-vararg,-cppcoreguidelines-pro-bounds-array-to-pointer-decay")

add_custom_target(
  clang-tidy
  COMMAND ${CLANG_TIDY}
  -p=${EXECUTABLE_OUTPUT_PATH}/../
  -export-fixes=${EXECUTABLE_OUTPUT_PATH}/../../code-checkers/clang-tidy-suggested-fixes.txt 
  -checks=${CLANG_TIDY_CHECKS}
  ${CODE_CHECK_SRC}
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
  ${CODE_CHECK_SRC}
)

