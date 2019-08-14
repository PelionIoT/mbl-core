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

# Given a payload tar and a component file name, extract the component,
# decompress it and write it to disk.
#
# $1: Tar archive file name
# $2: Component file name to extract and write to disk
# $3: Offset in KiB
extract_and_write_update_component() {
local payload="$1"
local component_filename="$2"
local offset_bytes=$(expr "$3" \* 1024)

# Find the disk name in the blockdev output
local disk_name=$(blockdev --report | grep \/dev\/mmcblk[0-9]$ | awk '{print $7}')

# Set the block size to the sector size of the disk
local block_size=$(blockdev --getss "$disk_name")

tar -xf "$payload" "$component_filename"
gunzip -c "$component_filename" | dd of=/dev/mmcblk1 seek="$offset_bytes" bs="$block_size"
}

# Remove the "do not reboot" flag
# Exits on errors.
#
# $1: The path to the reboot flag file
remove_do_not_reboot_flag_or_die() {
local dnr_flag_path="$1"

if ! rm -f "$dnr_flag_path"; then
    echo "Failed to remove ${dnr_flag_path} flag file";
    exit 27
fi
}

# ------------------------------------------------------------------------------
# Main code starts here
# ------------------------------------------------------------------------------

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
PART_INFO_FILES_DIR=/config/factory/part-info

# expected bootloader component file names
WKS_BL1_FILENAME=MBL_WKS_BOOTLOADER1
WKS_BL2_FILENAME=MBL_WKS_BOOTLOADER2


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

elif BOOTLOADER2_FILE=$(echo "${FIRMWARE_FILES}" | grep "${WKS_BL2_FILENAME}"); then

    # ------------------------------------------------------------------------------
    # Write a bootloader 2 component from an update payload file to disk
    # ------------------------------------------------------------------------------
    # Find the offset from the 'part-info' file
    OFFSET=$(cat "${PART_INFO_FILES_DIR}"/MBL_WKS_BOOTLOADER2_OFFSET_BANK1_KiB)

    extract_and_write_update_component "$FIRMWARE" "$BOOTLOADER2_FILE" "$OFFSET"
    sync

    # Remove the do not reboot flag, the user will likely want to reboot after applying
    # the bootloader update.
    remove_do_not_reboot_flag_or_die "${TMP_DIR}/do_not_reboot"

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
    remove_do_not_reboot_flag_or_die "${TMP_DIR}/do_not_reboot"
    exit 0

else

   echo "Failed to update firmware - rootfs.tar.xz is missing from archive file.";
   exit 28

fi
