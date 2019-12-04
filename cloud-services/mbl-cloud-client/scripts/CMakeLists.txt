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

set(BOOTFLAGS_DIR "/var/bootflags" CACHE PATH "Path to directory containing boot flag files")
set(TMP_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store small files")
set(UPDATE_PAYLOAD_DIR "/tmp" CACHE PATH "Path to directory used to temporarily store update payload files")
set(PAYLOAD_TMP_DIR "${UPDATE_PAYLOAD_DIR}/arm_update_activate" CACHE PATH "Path to directory used to temporarily store payload files")
set(LOG_DIR "/var/log" CACHE FILEPATH "Path to directory in which to write log files")
set(PART_INFO_DIR "/config/factory/part-info" CACHE STRING "Directory containing partition info files"

# Replace config value placeholders with values from CMake variables.
configure_file("arm_update_common.sh.in" "arm_update_common.sh" @ONLY)
configure_file("swupdate_handlers.lua" "swupdate_handlers.lua" @ONLY)
# We don't currently need to do any placeholder replacement in these scripts,
# but for consistency, copy them to the build directory.
configure_file("arm_update_activate.sh" "arm_update_activate.sh" COPYONLY)
configure_file("arm_update_active_details.sh" "arm_update_active_details.sh" COPYONLY)
configure_file("bootloader_installer.sh" "bootloader_installer.sh" COPYONLY)
configure_file("boot_partition_installer.sh" "boot_partition_installer.sh" COPYONLY)
configure_file("apps_installer.sh" "apps_installer.sh" COPYONLY)
