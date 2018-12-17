#!/bin/sh
# ----------------------------------------------------------------------------
# Copyright 2016-2018 ARM Ltd.
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


# ------------------------------------------------------------------------------
# Main code starts here
# ------------------------------------------------------------------------------

# The udpate payload ($FIRMWARE) is a tar file containing files for updates for
# different components of the system. Updates for different components are
# distinguished by their file names within the tar file.

# We only have to reboot during the update if there's an update for a component
# that requires a reboot, so set the do_not_reboot flag here and let it be
# deleted if a reboot is required..
touch /tmp/do_not_reboot

# Check that udpate payload ($FIRMWARE) is a tar file
if ! echo "${FIRMWARE}" | grep .tar$; then
    echo "Firmware update failed, \"${FIRMWARE}\" is not a tar file"
    exit 48
fi

# Check that udpate payload ($FIRMWARE) is not empty
tar_list_content_cmd="tar -tf"
if ! FIRMWARE_FILES=$(${tar_list_content_cmd} "${FIRMWARE}" | sed -e 's/\s\+/\n/g'); then
    echo "${tar_list_content_cmd} \"${FIRMWARE}\" failed!"
    exit 46
fi

# Check if we need to do firmware update or application update

# Make sure that we have only IPK file(s) in a root directory of the tar
if echo "${FIRMWARE_FILES}" | grep .ipk$; then

    # Check that udpate payload contains only IPK files
    if OTHER_FILES=$(echo "${FIRMWARE_FILES}" | grep -v .ipk$); then
        echo "${tar_list_content_cmd} \"${FIRMWARE}\" failed, there are non IPK files \"${OTHER_FILES}\" in udpate payload!"
        exit 49
    fi

    # Check that tar doesn't contain directories
    if echo "${FIRMWARE_FILES}" | grep /; then
        echo "${tar_list_content_cmd} \"${FIRMWARE}\" failed, IPK file(s) should be located in a root directory!"
        exit 50
    fi

    # ------------------------------------------------------------------------------
    # Install app updates from payload file
    # ------------------------------------------------------------------------------

    printf "%s\n" "${FIRMWARE}" > /scratch/firmware_path
    touch /scratch/do_app_update
    set +x
    echo "Waiting for app update to finish"
    while ! [ -e /scratch/done_app_update ]; do
        sleep 1
    done
    echo "App update finished"
    set -x
    rm /scratch/done_app_update

    app_update_rc=$(cat /scratch/app_update_rc)
    rm /scratch/app_update_rc
    if [ "$app_update_rc" -ne 0 ]; then
        exit 47
    fi
    save_header_or_die "$HEADER"
    exit 0

elif echo "${FIRMWARE_FILES}" | grep '^rootfs\.tar\.xz$'; then

    # Check that udpate payload contains only rootfs.tar.xz
    if OTHER_FILES=$(echo "${FIRMWARE_FILES}" | grep -v '^rootfs\.tar\.xz$'); then
        echo "${tar_list_content_cmd} \"${FIRMWARE}\" failed, udpate payload contains \"${OTHER_FILES}\" "
        exit 51
    fi

    # ------------------------------------------------------------------------------
    # Install rootfs update from payload file
    # ------------------------------------------------------------------------------
    # Detect root partitions
    root1=$(get_device_for_label rootfs1)
    exit_on_error "$?"

    root2=$(get_device_for_label rootfs2)
    exit_on_error "$?"

    FLAGS=$(get_device_for_label bootflags)
    exit_on_error "$?"

    # Find the partition that is currently mounted to /
    activePartition=$(get_active_root_device)

    if [ "$activePartition" = "$root1" ]; then
        activePartitionLabel=rootfs1
        nextPartition="$root2"
        nextPartitionLabel=rootfs2
    elif [ "$activePartition" = "$root2" ]; then
        activePartitionLabel=rootfs2
        nextPartition="$root1"
        nextPartitionLabel="rootfs1"
    else
        echo "Current root partition does not have a \"rootfs1\" or \"rootfs2\" label"
        exit 21
    fi

    create_root_partition_or_die "$nextPartition" "$nextPartitionLabel" "$FIRMWARE" "$ROOTFS_FILE"
    ensure_not_mounted_or_die "$nextPartition"

    ensure_mounted_or_die "${FLAGS}" /mnt/flags "$FLAGSFS_TYPE"

    save_header_or_die "$HEADER"
    sync

    if [ -e "/mnt/flags/${activePartitionLabel}" ]; then
        if ! mv "/mnt/flags/${activePartitionLabel}" "/mnt/flags/${nextPartitionLabel}"; then
            echo "Failed to rename boot flag file";
            exit 25
        fi
    else
        if ! touch "/mnt/flags/${nextPartitionLabel}"; then
            echo "Failed to create new boot flag file";
            exit 26
        fi
    fi
    sync

    # Remove do_not_reboot_flag - we need a reboot for rootfs updates
    if ! rm -f /tmp/do_not_reboot; then
        echo "Failed to remove /tmp/do_not_reboot flag file";
        exit 27
    fi

    exit 0

else

   echo "Failed to update firmware - invalid update content";
   exit 28

fi
