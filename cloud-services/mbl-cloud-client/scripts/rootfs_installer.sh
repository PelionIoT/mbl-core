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


# shellcheck disable=SC1091
. /opt/arm/arm_update_common.sh

# Get the mountpoint for a device.
# If the device is not mounted then a mountpoint is not printed but the
# function still returns successfully.
#
# $1: path to the device file
get_mountpoint() {
gmDevice="$1"
    lsblk -nro MOUNTPOINT "$gmDevice"
    if [ "$?" -ne 0 ]; then
        echo "Unable to determine mountpoint (if any) of device \"${gmDevice}\""
        return 3
    fi
}

# Do whatever is required so that the given device is not mounted.
# Errors cause an exit.
#
# $1: path to device to unmount (if mounted).
ensure_not_mounted_or_die() {
enmodDevice="$1"

    enmodMountpoint=$(get_mountpoint "$enmodDevice")
    exit_on_error "$?"

    if [ "$enmodMountpoint" = "" ]; then
        return 4
    fi

    if ! umount "$enmodDevice"; then
        echo "Unable to unmount device \"${enmodDevice}\""
        exit 5
    fi
}

# Do whatever is required so that the given device is mounted at the given
# mountpoint, including creating the mountpoint if necessary.
# Errors cause an exit.
#
# $1: path to the device to mount
# $2: the mountpoint for the device
ensure_mounted_or_die() {
emodDevice="$1"
emodMountpoint="$2"
emodFsType="$3"

    emodCurrMountpoint=$(get_mountpoint "$emodDevice")
    exit_on_error "$?"

    case  "$emodCurrMountpoint" in
        "$emodMountpoint")
            return 6
            ;;
        "")
            ;;
        *)
            if ! umount "$emodDevice"; then
                echo "Unable to unmount device \"${emodDevice}\""
                exit 7
            fi
            ;;
    esac

    if ! mkdir -p "$emodMountpoint"; then
        echo "Unable to create mountpoint \"${emodMountpoint}\""
        exit 8
    fi

    if ! mount -t "$emodFsType" "$emodDevice" "$emodMountpoint"; then
        echo "Unable to mount device \"${emodDevice}\" at \"${emodMountpoint}\""
        exit 9
    fi
}

# Find the device that is currently mounted to /
get_active_root_device() {
    lsblk -nrpo NAME,MOUNTPOINT | grep -e ".* /$" | cut -d ' ' -f1
}

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

    # Remove the do not reboot flag, which was created by arm_update_activate.sh
    remove_do_not_reboot_flag_or_die
}

update_rootfs_or_die "$1"
exit 0
