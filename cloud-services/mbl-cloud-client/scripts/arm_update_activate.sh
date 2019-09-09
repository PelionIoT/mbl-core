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
# Write a partition image file to a partition and set the boot flags.

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
# $1: Path to the device file for the new root partition
# $2: Label for the new root partition
# $3: Update payload file (tar file) containing a rootfs tar.xz file to
#     populate the new partition
# $4: Name of the rootfs archive within the payload tar file
create_root_partition_or_die() {
crpodPart="$1"
crpodLabel="$2"
crpodPayloadFile="$3"
crpodRootfsFile="$4"

    crpodMountpoint=/mnt/root
    format_and_mount_root_partition_or_die "$crpodPart" "$crpodLabel" "$crpodMountpoint"

    if EXTRACT_UNSAFE_SYMLINKS=1 tar -xOf "$crpodPayloadFile" "$crpodRootfsFile" | tar -xJ -C "$crpodMountpoint"; then
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
}

# Check IPK file name validity
#
# $1: IPK full file name
check_ipk_filename_validity() {
ipk_filename="$1"
    # Check IPK extension
    file_extension="${ipk_filename##*.}"
    if [ "$file_extension" != "ipk" ]; then
        printf "Check IPK tar filename validity failed: there is a non IPK file %s in udpate payload!\n" "${ipk_filename}"
        exit 50
    fi
    ipk_directory_name=$(dirname "${ipk_filename}")
    if ! real_ipk_directory_name=$(realpath "${ipk_directory_name}" 2>/dev/null); then
        echo "realpath failed: tar includes file in not existing directory"
        exit 52
    fi
    if [ "$real_ipk_directory_name" != "$PWD" ]; then
        printf "IPK directory %s different from the current directory %s. This can happen only if IPK doesn't reside in a root directory of tar" "${real_ipk_directory_name}" "${PWD}"
        exit 53
    fi
}

tar_list_content_cmd="tar -tf"

# Check IPK tar validity
#
# $1: Tar file name
check_ipk_tar_validity() {
tar_filename="$1"
    tar_file_list=$(${tar_list_content_cmd} "$tar_filename")
    for file_in_tar in ${tar_file_list}; do
        check_ipk_filename_validity "${file_in_tar}"
    done
}

# Validate an integer is positive
# Exits on errors
#
# $1: Integer to validate
validate_positive_integer_or_die() {
    if ! printf '%s\n' "$1"  | grep '^[0-9]\+$'; then
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

    if ! rootfs_part=$(blkid -L "$gdnflod_label"); then
        printf "blkid failed to determine the device file for the partition with label %s.\n" "$gdnflod_label"
        exit 54
    fi

    if ! gdnflod_disk_name=$(printf '%s\n' "$rootfs_part" | sed 's/p[0-9]\+$//'); then
        printf "Failed to strip the partition number from the root partition's device file name.\n"
        exit 55
    fi

    if [ "$gdnflod_disk_name" = "$rootfs_part" ]; then
        printf "Failed to strip the partition number from the root partition's device file name: %s\n" "$gdnflod_disk_name"
        exit 56
    fi

    printf "%s\n" "$gdnflod_disk_name"
}

# Given a payload tar and a component file name, extract the component,
# decompress it and write it to disk.
#
# $1: Tar archive file name
# $2: Component file name
# $3: Offset in KiB
# $4: Expected size in KiB
# $5: Boot mount point (empty string if not writing to a boot partition)
# $6: BLFS mount point (empty string if not writing to the blfs partition)
extract_and_write_update_component_or_die() {
ewuc_payload="$1"
ewuc_component_filename="$2"
ewuc_boot_part_mnt_point="$5"
ewuc_fs_part_mnt_point="$6"
    # shellcheck disable=SC2003
    ewuc_offset_bytes=$(expr "$3" \* 1024)
    # shellcheck disable=SC2003
    ewuc_max_img_size=$(expr "$4" \* 1024)

    ewuc_disk_name=$(get_disk_name_from_label_or_die "$ROOTFS1_LABEL")
    # get_disk_name_from_label_or_die runs in a subshell here so if it exits it
    # will only exit from the subshell, not stop this function executing. Check
    # the subshell's exit code so that we can propagate the "exit" if required.
    ewuc_err=$?
    if [ "$ewuc_err" -ne "0" ]; then
        exit "$ewuc_err"
    fi

    if ! mkdir -p "$UPDATE_PAYLOAD_DIR/tmp"; then
        printf "Failed to create temporary directory at %s\n" "$UPDATE_PAYLOAD_DIR/tmp"
    fi

    if ! tar -xvf "$ewuc_payload" "$ewuc_component_filename" -C "$UPDATE_PAYLOAD_DIR/tmp"; then
        printf "Failed to extract %s from %s\n" "$ewuc_component_filename" "$ewuc_payload"
        exit 57
    fi

    if ! tar -xvzf "$UPDATE_PAYLOAD_DIR/tmp/$ewuc_component_filename" -C "$UPDATE_PAYLOAD_DIR"; then
        printf "Failed to extract and decompress %s\n" "$ewuc_component_filename"
        exit 58
    fi
    # shellcheck disable=SC2003
    if ! ewuc_actual_img_size=$(expr "$(du -k "$UPDATE_PAYLOAD_DIR/$ewuc_component_filename" | awk '{print $1}')" \* 1024); then
        printf "Failed to get the size of \"%s\"\n" "$ewuc_component_filename"
        exit 59
    fi

    validate_positive_integer_or_die "$ewuc_actual_img_size"

    if [ "$ewuc_actual_img_size" -gt "$ewuc_max_img_size" ]; then
        printf "Image size is greater than the maximum allocated size: Actual size %s. Expected size: %s\n" "$ewuc_actual_img_size" "$ewuc_max_img_size"
        exit 60
    fi

    # Copy files to the boot partition, and the bootloader FS partition if it exists
    if [ ! -z "$ewuc_boot_part_mnt_point"  ]; then
        if [ ! -z "$ewuc_fs_part_mnt_point" ]; then
            copy_dir_or_die "$UPDATE_PAYLOAD_DIR/$ewuc_component_filename" "$ewuc_fs_part_mnt_point"
        fi

        copy_dir_or_die "$UPDATE_PAYLOAD_DIR/$ewuc_component_filename" "$ewuc_boot_part_mnt_point"
    else
        # Write the file to raw flash.
        #
        # Linux always considers the sector size to be 512 bytes, no matter what
        # the device's actual block size is. We just let dd use its default block size and
        # ensure the seek is always a byte count.
        # See: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/linux/types.h?id=v4.4-rc6#n121
        if ! dd if="$UPDATE_PAYLOAD_DIR/$ewuc_component_filename" of="$ewuc_disk_name" oflag=seek_bytes conv=fsync seek="$ewuc_offset_bytes"; then
            # We might be on raspberry pi 3, which has some version of dd without conv and oflag options
            # shellcheck disable=SC2003
            # shellcheck disable=SC1001
            if ! dd if="$UPDATE_PAYLOAD_DIR/$ewuc_component_filename" of="$ewuc_disk_name" seek="$(expr "$ewuc_offset_bytes" \/ 512)"; then
                printf "Writing %s to disk failed.\n" "$ewuc_component_filename"
                exit 63
            fi
        fi
    fi

    # Clean up.
    if ! rm -rf "$UPDATE_PAYLOAD_DIR/tmp"; then
        print "Failed to remove temporary directory after update %s\n" "$UPDATE_PAYLOAD_DIR/tmp"
    fi

    if ! rm -rf "${UPDATE_PAYLOAD_DIR:?}/$ewuc_component_filename"; then
        printf "Failed to remove the decompressed files \"%s\" after update\n" "$ewuc_component_filename"
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

# Given a payload tar, ensure that its format version is supported
#
# $1: path to payload tar file.
ensure_payload_format_support_or_die() {
gpfvod_payload="$1"

    if ! gpfvod_format_version=$(tar -xf "$gpfvod_payload" payload_format_version -O); then
        printf "Failed to extract payload format version from payload file \"%s\"\n" "$gpfvod_payload"
        exit 29
    fi

    printf "Payload format version is \"%s\"\n" "$gpfvod_format_version"

    if [ "$gpfvod_format_version" != "1" ]; then
        printf "Payload format version \"%s\" is not supported" "$gpfvod_format_version"
        exit 30
    fi
}

# ------------------------------------------------------------------------------
# Main code starts here
# ------------------------------------------------------------------------------

ensure_payload_format_support_or_die "$FIRMWARE"

# The udpate payload ($FIRMWARE) is a tar file containing files for updates for
# different components of the system. Updates for different components are
# distinguished by their file names within the tar file.

# We only have to reboot during the update if there's an update for a component
# that requires a reboot, so set the do_not_reboot flag here and let it be
# deleted if a reboot is required..
touch "${TMP_DIR}/do_not_reboot"

# list files in udpate payload ($FIRMWARE) tar file
FIRMWARE_FILES=$(${tar_list_content_cmd} "${FIRMWARE}")

# directory containing files which contain information about the update payloads
PART_INFO_FILES_DIR="${FACTORY_CONFIG_PARTITION}"/part-info

# patterns to match bootloader component file names
WKS_BOOTLOADER_FILENAME_RE='^WKS_BOOTLOADER[0-9].*\.tar\.gz$'
WKS_IMAGE_BOOT_FILES_RE='^BOOT.*\.tar\.gz$'

# Check if we need to do firmware update or application update
if echo "${FIRMWARE_FILES}" | grep .ipk$; then

    # Check IPK tar validity: all files inside tar shoulf reside in a root of tar arcive
    check_ipk_tar_validity "${FIRMWARE}"

    # ------------------------------------------------------------------------------
    # Install app updates from payload file
    # ------------------------------------------------------------------------------

    printf "%s\n" "${FIRMWARE}" > "${UPDATE_PAYLOAD_DIR}/firmware_path"
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
    save_header_or_die "$HEADER"

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

    exit "$?"

elif BOOTLOADER_FILES=$(echo "${FIRMWARE_FILES}" | grep "${WKS_BOOTLOADER_FILENAME_RE}\|${WKS_IMAGE_BOOT_FILES_RE}"); then

    # ------------------------------------------------------------------------------
    # Write one or more bootloader components from an update payload file to disk
    # ------------------------------------------------------------------------------
    # variable is unquoted to remove carriage returns, tabs, multiple spaces etc
    for bl_file in $BOOTLOADER_FILES; do
        bl_filename_no_suffix=$(printf "%s\n" "$bl_file" | sed 's/\.tar.gz$//')
        bl_part_info_file_path_prefix="${PART_INFO_FILES_DIR}/${bl_filename_no_suffix}"
        size=$(cat "${bl_part_info_file_path_prefix}_SIZE_KiB")

        if printf "%s\n" "$bl_file" | grep "${WKS_IMAGE_BOOT_FILES_RE}"; then
            if ! boot_mount_point=$(cat "${PART_INFO_FILES_DIR}/MBL_BOOT_MOUNT_POINT"); then
                printf "Failed to find the boot partition mount point from partition info file: %s.\n" "${bl_part_info_file_path_prefix}_MOUNT_POINT"
                exit 31
            fi

            if [ "$(cat "${PART_INFO_FILES_DIR}/MBL_WKS_BOOTLOADER_FS_SKIP")" = "0" ]; then
                fs_mount_point=$(cat "${PART_INFO_FILES_DIR}/MBL_WKS_BOOTLOADER_FS_MOUNT_POINT")
            fi
        else
            validate_positive_integer_or_die "$size"

            # We're writing to a raw partition so we need the offset
            offset=$(cat "${bl_part_info_file_path_prefix}_OFFSET_BANK1_KiB")
            validate_positive_integer_or_die "$offset"
        fi
        # If this cat fails the 'SKIP' file does not exist and the check
        # below will evaluate to false as should_skip will be empty.
        should_skip=$(cat "${bl_part_info_file_path_prefix}_SKIP")
        if [ "$should_skip" = "1" ]; then
            printf "Partition is marked as skipped. Exiting.\n"
            exit 32
        fi

        extract_and_write_update_component_or_die "$FIRMWARE" "$bl_file" "$offset" "$size" "$boot_mount_point" "$fs_mount_point"
        sync
    done

    # Remove the do not reboot flag, the user will likely want to reboot after applying
    # the bootloader update.
    remove_do_not_reboot_flag_or_die
    exit 0

elif ROOTFS_FILE=$(echo "${FIRMWARE_FILES}" | grep '^rootfs\.tar\.xz$'); then

    # ------------------------------------------------------------------------------
    # Install rootfs update from payload file
    # ------------------------------------------------------------------------------
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

    create_root_partition_or_die "$nextPartition" "$nextPartitionLabel" "$FIRMWARE" "$ROOTFS_FILE"
    ensure_not_mounted_or_die "$nextPartition"

    save_header_or_die "$HEADER"
    sync

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

    # Remove do_not_reboot_flag - we need a reboot for rootfs updates
    remove_do_not_reboot_flag_or_die
    exit 0

else

   echo "Failed to update firmware - rootfs.tar.xz is missing from archive file.";
   exit 28

fi
