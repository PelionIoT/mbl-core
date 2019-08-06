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
set(LOG_DIR "/var/log" CACHE FILEPATH "Path to directory in which to write log files")
set(ROOTFS1_LABEL "rootfs1" CACHE STRING "Label used for the first root file system bank")
set(ROOTFS2_LABEL "rootfs2" CACHE STRING "Label used for the second root file system bank")
set(ROOTFS_TYPE "ext4" CACHE STRING "Type of file system used for root file systems")

# Replace config value placeholders with values from CMake variables.
configure_file("arm_update_common.sh.in" "arm_update_common.sh" @ONLY)

# We don't currently need to do any placeholder replacement in these scripts,
# but for consistency, copy them to the build directory.
configure_file("arm_update_activate.sh" "arm_update_activate.sh" COPYONLY)
configure_file("arm_update_active_details.sh" "arm_update_active_details.sh" COPYONLY)
