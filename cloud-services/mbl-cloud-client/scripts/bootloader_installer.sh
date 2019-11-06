#!/bin/sh
# ----------------------------------------------------------------------------
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ----------------------------------------------------------------------------


. /opt/arm/arm_update_common.sh

# Write a file to raw storage on the same device as the rootfs.
# Exits on errors.
#
# $1: Path to input file.
# $2: Offset into flash device in bytes.
# $3: Max size to write in bytes.
write_to_flash_or_die() {
wfod_file="$1"
wfod_offset_B="$2"
wfod_max_size_B="$3"

    wfod_disk_name=$(get_disk_name_from_label_or_die "$ROOTFS1_LABEL")
    exit_on_error "$?"

    if ! wfod_actual_size_B=$(wc -c < "$wfod_file"); then
        printf "Failed to get the size of \"%s\"\n" "$wfod_file"
        exit 59
    fi

    if [ "$wfod_actual_size_B" -gt "$wfod_max_size_B" ]; then
        printf "Image size is greater than the maximum allocated size: Actual size %s. Expected size: %s\n" "$wfod_actual_size_B" "$wfod_max_size_B"
        exit 60
    fi

    # Write the file to raw flash.
    #
    # Linux always considers the sector size to be 512 bytes, no matter what
    # the device's actual block size is. We just let dd use its default block size and
    # ensure the seek is always a byte count.
    # See: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/types.h?id=v4.4-rc6#n121
    if ! dd if="$wfod_file" of="$wfod_disk_name" oflag=seek_bytes conv=fsync seek="$wfod_offset_B"; then
        printf "Writing \"%s\" to disk failed.\n" "$wfod_file"
        exit 63
    fi
    sync
}


# Update a bootloader component (WKS_BOOTLOADER1 or WKS_BOOTLOADER2).
# Exits on failure.
#
# $1: path to the (xz compressed) bootloader image to write
update_bootloader_or_die() {
ublod_path="$1"

    ublod_basename=$(basename "$ublod_path" .xz)

    if is_part_skipped "$ublod_basename"; then
        printf "\"%s\" partition is marked as skipped. Exiting.\n" "$ublod_basename"
        exit 32
    fi

    ublod_max_size_B=$(get_part_size_B "$ublod_basename")
    exit_on_error "$?"

    ublod_offset_B=$(get_part_offset_B "$ublod_basename" 1)
    exit_on_error "$?"

    ublod_decompressed_path=${ublod_path%.xz}
    decompress_or_die "$ublod_path" "$ublod_decompressed_path"
    write_to_flash_or_die "$ublod_decompressed_path" "$ublod_offset_B" "$ublod_max_size_B"

    # Remove the do not reboot flag, which was created by arm_update_activate.sh, the user
    # will need to reboot after applying the bootloader update.
    remove_do_not_reboot_flag_or_die
}

update_bootloader_or_die "$1"
exit 0
