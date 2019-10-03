#!/bin/sh
# ----------------------------------------------------------------------------
# Copyright (c) 2016-2018 Arm Limited and Contributors. All rights reserved.
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

# Note - exit codes 1-20 come from arm_udpate_common.sh.

set -x

# Parse command line
#
# HEADER
# FIRMWARE
# LOCATION
# OFFSET
# SIZE
#
# shellcheck disable=SC1091
. /opt/arm/arm_update_cmdline.sh
# shellcheck disable=SC1091
. /opt/arm/arm_update_common.sh

if [ -n "$ARM_UPDATE_ACTIVATE_LOG_PATH" ]; then
    # Redirect stdout and stderr to the log file
    exec >>"$ARM_UPDATE_ACTIVATE_LOG_PATH" 2>&1
    printf "%s: %s\n" "$(date '+%FT%T%z')" "Starting arm_update_activate.sh"
fi

# Format and mount a root partition
# Exits on errors.
#
# $1: Path to the device file for the partition
# $2: Label to give the new partition
# $3: Mountpoint to use when mounting after formatting
format_and_mount_root_partition_or_die() {
famrpodPart="$1"
famrpodLabel="$2"
famrpodMountpoint="$3"

    ensure_not_mounted_or_die "$famrpodPart"

    # Create the file system on the next partition
    if ! mkfs -t "$ROOTFS_TYPE" -L "$famrpodLabel" -F "$famrpodPart"; then
        echo "mkfs failed on the new root partition"
        exit 22
    fi

    ensure_mounted_or_die "$famrpodPart" "$famrpodMountpoint" "$ROOTFS_TYPE"
}

# Formats and populates a new root partition
# Exits on errors.
#
# $1: path to rootfs archive
# $2: Path to the device file for the new root partition
# $3: Label for the new root partition
create_root_partition_or_die() {
crpodRootfsFile="$1"
crpodPart="$2"
crpodLabel="$3"

    crpodMountpoint=/mnt/root
    format_and_mount_root_partition_or_die "$crpodPart" "$crpodLabel" "$crpodMountpoint"

    if tar -xf "$crpodRootfsFile" -C "$crpodMountpoint"; then
        return 0
    fi
    echo "Failed to unarchive rootfs update"
    exit 23
}

# Save the update header to a standard place
#
# $1: Header to save
save_header_or_die() {
shodHeader="$1"
    if ! cp "$shodHeader" "$SAVED_HEADER_PATH"; then
        echo "Failed to copy image header"
        exit 24
    fi
    sync
}

# Validate an integer is positive
# Exits on errors
#
# $1: Integer to validate
validate_positive_integer_or_die() {
    if ! printf '%s\n' "$1"  | grep -q '^[0-9]\+$'; then
        printf "%s is not a positive integer!\n" "$1"
        exit 64
    fi
}

# Copy all files in a directory from src to dst, remounting the file system as rw before copying.
# Exits on errors.
#
# $1: Source path
# $2: Destination mount point
copy_dir_or_die() {
cfod_source_path="$1"
cfod_mnt_point="$2"

    remount_partition_or_die "$cfod_mnt_point" rw

    if ! rm -f "$cfod_mnt_point"/*; then
        printf "Failed to remove files on partition %s\n" "$cfod_mnt_point"
        exit 60
    fi

    if ! cp -f "$cfod_source_path"/* "$cfod_mnt_point"; then
        printf "Copying %s to %s failed.\n" "$cfod_source_path" "$cfod_mnt_point"
        exit 61
    fi

    remount_partition_or_die "$cfod_mnt_point" ro
}

# Get the disk name from the device label.
# Exits on errors.
#
# $1: Device label
get_disk_name_from_label_or_die() {
gdnflod_label="$1"

    if ! gdnflod_part_for_label=$(blkid -L "$gdnflod_label"); then
        printf "blkid failed to determine the device file for the partition with label %s.\n" "$gdnflod_label"
        exit 54
    fi

    if ! gdnflod_disk_name=$(printf '%s\n' "$gdnflod_part_for_label" | sed 's/p[0-9]\+$//'); then
        printf "Failed to strip the partition number from the root partition's device file name.\n"
        exit 55
    fi

    if [ "$gdnflod_disk_name" = "$gdnflod_part_for_label" ]; then
        printf "Failed to strip the partition number from the root partition's device file name: %s\n" "$gdnflod_disk_name"
        exit 56
    fi

    printf "%s\n" "$gdnflod_disk_name"
}

# Decompress a .xz file.
# Exits on errors.
#
# $1: Path to compressed file.
# $2: Path to write decompressed file to.
decompress_or_die() {
dod_compressed_path="$1"
dod_decompressed_path="$2"
    if ! xzcat "$dod_compressed_path" > "$dod_decompressed_path"; then
        printf "Failed to decompress \"%s\" to \"%s\"\n" "$dod_compressed_path" "$dod_decompressed_path"
        exit 58
    fi
}

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

# Check if a partition exists on this device.
# Returns zero if partition is skipped; non-zero otherwise
#
# $1: Name of partition (e.g. WKS_BOOTLOADER_FS)
is_part_skipped() {
ips_part_name="$1"
    [ "$(cat "${PART_INFO_FILES_DIR}/MBL_${ips_part_name}_SKIP")" = "1" ]
}

# Get a size variable (either SIZE or OFFSET) for a partition in bytes.
# Exits on failures (but you're going to run this function in a subshell so you
# still have to check the error code).
#
# $1: Name of partition (e.g. WKS_BOOTLOADER_FS)
# $2: Name of variable (SIZE or OFFSET1)
get_part_size_var_B() {
gpsv_part_name="$1"
gpsv_var_name="$2"

    gpsv_size_KiB=$(cat "${PART_INFO_FILES_DIR}/MBL_${gpsv_part_name}_${gpsv_var_name}_KiB")
    validate_positive_integer_or_die "$gpsv_size_KiB"

    # shellcheck disable=SC2003
    gpsv_size_B=$(expr "$gpsv_size_KiB" \* 1024)
    validate_positive_integer_or_die "$gpsv_size_B"

    printf "%s\n" "$gpsv_size_B"
    return 0
}

# Get the size of a partition in bytes.
# Exits on failures (but you're going to run this function in a subshell so you
# still have to check the error code).
#
# $1: Name of partition (e.g. WKS_BOOTLOADER_FS)
get_part_size_B() {
gps_part_name="$1"
    get_part_size_var_B "$gps_part_name" SIZE
}

# Get the offset of a partition in bytes.
# Exits on failures (but you're going to run this function in a subshell so you
# still have to check the error code).
#
# $1: Name of partition (e.g. WKS_BOOTLOADER_FS)
# $2: Bank number (default: 1)
get_part_offset_B() {
gpo_part_name="$1"
gpo_bank="${2:-1}"

    get_part_size_var_B "$gpo_part_name" "OFFSET_BANK${gpo_bank}"
}

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

    # Remove the do not reboot flag, the user will need to reboot after
    # applying the bootloader update.
    remove_do_not_reboot_flag_or_die
}

# Update the apps component.
# Exits on failure.
#
# $1: path of (xz compressed) archive containing apps
update_apps_or_die() {
uaod_apps_file="$1"

    printf "%s\n" "${uaod_apps_file}" > "${UPDATE_PAYLOAD_DIR}/firmware_path"
    touch "${UPDATE_PAYLOAD_DIR}/do_app_update"
    set +x
    echo "Waiting for app update to finish"
    while ! [ -e "${UPDATE_PAYLOAD_DIR}/done_app_update" ]; do
        sleep 1
    done
    echo "App update finished"
    set -x
    rm "${UPDATE_PAYLOAD_DIR}/done_app_update"

    app_update_rc=$(cat "${UPDATE_PAYLOAD_DIR}/app_update_rc")
    rm "${UPDATE_PAYLOAD_DIR}/app_update_rc"
    if [ "$app_update_rc" -ne 0 ]; then
        exit 47
    fi

    # mbed-cloud-client does not currently support component update; it
    # therefore expects the system receiving the update to be rebooted as
    # a whole image is expected to have been updated.
    # The registration process that kicks in at boot up notifies Pelion Device
    # Management that the update was successfully applied.
    #
    # Until component updates is implemented, it is required to restart the
    # instance of the client running as service in order to simulate the
    # expected system reboot.
    #
    # It was deemed safe to reboot at this stage as no other step is expected
    # at this point other than rebooting.
    systemctl restart mbl-cloud-client
    exit_on_error "$?"
}

# Update the rootfs component.
# Exits on failure.
#
# $1: Path to the (xz compressed) archive containing new rootfs contents
update_rootfs_or_die() {
urod_rootfs_file="$1"

    # Detect root partitions
    root1=$(get_device_for_label "$ROOTFS1_LABEL")
    exit_on_error "$?"

    root2=$(get_device_for_label "$ROOTFS2_LABEL")
    exit_on_error "$?"

    # Find the partition that is currently mounted to /
    activePartition=$(get_active_root_device)

    if [ "$activePartition" = "$root1" ]; then
        activePartitionLabel="$ROOTFS1_LABEL"
        nextPartition="$root2"
        nextPartitionLabel="$ROOTFS2_LABEL"
    elif [ "$activePartition" = "$root2" ]; then
        activePartitionLabel="$ROOTFS2_LABEL"
        nextPartition="$root1"
        nextPartitionLabel="$ROOTFS1_LABEL"
    else
        echo "Current root partition does not have a \"${ROOTFS1_LABEL}\" or \"${ROOTFS2_LABEL}\" label"
        exit 21
    fi

    create_root_partition_or_die "$urod_rootfs_file" "$nextPartition" "$nextPartitionLabel"
    ensure_not_mounted_or_die "$nextPartition"

    if [ -e "${FLAGS_DIR}/${activePartitionLabel}" ]; then
        if ! mv "${FLAGS_DIR}/${activePartitionLabel}" "${FLAGS_DIR}/${nextPartitionLabel}"; then
            echo "Failed to rename boot flag file";
            exit 25
        fi
    else
        if ! touch "${FLAGS_DIR}/${nextPartitionLabel}"; then
            echo "Failed to create new boot flag file";
            exit 26
        fi
    fi
    sync

    remove_do_not_reboot_flag_or_die
}

# Extract all files from an update payload.
# Exits on failure.
#
# $1: path to the payload file.
# $2: destination directory.
extract_all_files_from_payload_or_die() {
ecfp_payload_path="$1"
ecfp_destination="$2"
    if ! (cd "$ecfp_destination" && cpio -iudF "$ecfp_payload_path"); then
        printf "Failed to extract files from update payload \"%s\"\n" "$ecfp_payload_path"
        exit 57
    fi
}

# Remove the "do not reboot" flag
# Exits on errors.
remove_do_not_reboot_flag_or_die() {
    dnr_flag_path="${TMP_DIR}/do_not_reboot"

    if ! rm -f "$dnr_flag_path"; then
        printf "Failed to remove the \"%s\" flag file\n" "$dnr_flag_path"
        exit 27
    fi
}

# Given a payload_format_version file, ensure that the payload format is
# supported.
# Exits on errors.
#
# $1: path to the payload_format_version file
ensure_payload_format_support_or_die() {
gpfvod_format_version_file="$1"

    if ! gpfvod_format_version=$(cat "$gpfvod_format_version_file"); then
        printf "Failed to get payload format version from file \"%s\"\n" "$gpfvod_format_version_file"
        exit 29
    fi

    printf "Payload format version is \"%s\"\n" "$gpfvod_format_version"

    if [ "$gpfvod_format_version" != "2" ]; then
        printf "Payload format version \"%s\" is not supported" "$gpfvod_format_version"
        exit 30
    fi
}

# Set up the temporary directory used for storing component payloads
# Exits on errors
set_up_payload_tmp_dir_or_die() {
    if ! rm -rf "$PAYLOAD_TMP_DIR"; then
        printf "Failed to remove previously existing tmp dir \"%s\"\n" "$PAYLOAD_TMP_DIR"
        exit 33
    fi
    if ! mkdir -p "$PAYLOAD_TMP_DIR"; then
        printf "Failed to create tmp dir \"%s\"\n" "$PAYLOAD_TMP_DIR"
        exit 34
    fi
}

# Remove temporary files.
# To be called from exit handler
clean_up() {
    rm -rf "$PAYLOAD_TMP_DIR"
}

# ------------------------------------------------------------------------------
# Main code starts here
# ------------------------------------------------------------------------------

if ! FIRMWARE_PATH=$(realpath "$FIRMWARE"); then
    printf "Failed to resolve path to payload file \"%s\"\n" "$FIRMWARE"
    exit 66
fi

PAYLOAD_TMP_DIR="${UPDATE_PAYLOAD_DIR}/arm_update_activate"
set_up_payload_tmp_dir_or_die
trap clean_up EXIT

extract_all_files_from_payload_or_die "$FIRMWARE_PATH" "$PAYLOAD_TMP_DIR"
ensure_payload_format_support_or_die "${PAYLOAD_TMP_DIR}/payload_format_version"

# We only have to reboot during the update if there's an update for a component
# that requires a reboot, so set the do_not_reboot flag here and let it be
# deleted if a reboot is required..
touch "${TMP_DIR}/do_not_reboot"

# list files from udpate payload
if ! FIRMWARE_FILES=$(find "${PAYLOAD_TMP_DIR}" -type f); then
    printf "Failed to list files from payload in \"%s\"\n" "$PAYLOAD_TMP_DIR"
    exit 67
fi

# directory containing files which contain information about the update payloads
PART_INFO_FILES_DIR="${FACTORY_CONFIG_PARTITION}"/part-info

for component_path in ${FIRMWARE_FILES}; do
    case "$component_path" in
        */WKS_BOOTLOADER1.xz)
            update_bootloader_or_die "$component_path"
            ;;
        */WKS_BOOTLOADER2.xz)
            update_bootloader_or_die "$component_path"
            ;;
        */BOOT.tar.xz)
            update_boot_or_die "$component_path"
            ;;
        */ROOTFS.tar.xz)
            update_rootfs_or_die "$component_path"
            ;;
        */APPS.tar.xz)
            update_apps_or_die "$component_path"
            ;;
        */payload_format_version)
            ;;
        *)
            printf "Skipping unrecognized file \"%s\" in payload \"%s\"\n" "$(basename "$component_path")" "$FIRMWARE_PATH"
            ;;
    esac
done
save_header_or_die "$HEADER"
exit 0
