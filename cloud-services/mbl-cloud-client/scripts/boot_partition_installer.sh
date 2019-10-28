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

# Update a "boot partition" with the contents of a directory.
# Exits on failure.
#
# $1: Name of partition (BOOT or WKS_BOOTLOADER_FS)
# $2: Directory holding the new contents for the partition
# $3: Estimated size in bytes of the new contents
# $4: Set to "yes" if non-existence of the partition is not a failure
update_boot_part_from_dir_or_die() {
ubpd_part_name="$1"
ubpd_dir="$2"
ubpd_size_estimate_B="$3"
ubpd_skip_ok="$4"

    if is_part_skipped "$ubpd_part_name"; then
        if [ "$ubpd_skip_ok" != "yes" ]; then
            printf "Partition \"%s\" does not exist\n" "$ubpd_part_name"
            exit 65
        fi
        return 0
    fi

    ubpd_max_size_B=$(get_part_size_B "$ubpd_part_name")
    exit_on_error "$?"

    if [ "$ubpd_size_estimate_B" -gt "$ubpd_max_size_B" ]; then
        printf "Estimated update payload size is greater than the \"%s\" partition size: payload size: %sB. partition size: %sB\n" \
            "$ubpd_part_name" \
            "$ubpd_size_estimate_B" \
            "$ubpd_max_size_B"
        exit 60
    fi

    ubpd_mount_point_file="${PART_INFO_FILES_DIR}/MBL_${ubpd_part_name}_MOUNT_POINT"
    if ! ubpd_mount_point=$(cat "$ubpd_mount_point_file") || [ -z "$ubpd_mount_point" ]; then
        printf "Failed to find the mount point for partition \"%s\" from partition info file \"%s\"\n" \
            "$ubpd_part_name" \
            "ubpd_mount_point_file"
        exit 31
    fi
    copy_dir_or_die "$ubpd_dir" "$ubpd_mount_point"

    remove_do_not_reboot_flag_or_die
}

# Update the boot partition.
# Exits on failure.
#
# $1: path to archive containing new contents
update_boot_or_die() {
ubod_path="$1"

    ubod_tmp_dir="${PAYLOAD_TMP_DIR}/boot"
    if ! mkdir -p "$ubod_tmp_dir"; then
        printf "Failed to create directory \"%s\"\n" "$ubod_tmp_dir"
        exit 62
    fi
    if ! tar -xf "$ubod_path" -C "$ubod_tmp_dir"; then
        printf "Failed to extract files from \"%s\"\n" "$ubod_path"
        exit 58
    fi

    # shellcheck disable=SC2003
    if ! ubod_size_estimate_B=$(expr "$(du -k "$ubod_tmp_dir" | awk '{print $1}')" \* 1024); then
        printf "Failed to get the size of \"%s\"\n" "$ubod_tmp_dir"
        exit 59
    fi
    validate_positive_integer_or_die "$ubod_size_estimate_B"

    update_boot_part_from_dir_or_die BOOT "$ubod_tmp_dir" "$ubod_size_estimate_B" no

    # On raspberrypi3 we have a bootloaderfs partition and a boot
    # partition. The bootloaderfs partition should hold the VC4 firmware
    # and TF-A BL2, and the boot partition should hold the kernel FIT
    # image.
    #
    # We have not yet split the contents of these two partitions apart
    # though, so currently we keep all files on both partitions.
    update_boot_part_from_dir_or_die WKS_BOOTLOADER_FS "$ubod_tmp_dir" "$ubod_size_estimate_B" yes

    remove_do_not_reboot_flag_or_die
}

update_boot_or_die "$1"
exit 0
