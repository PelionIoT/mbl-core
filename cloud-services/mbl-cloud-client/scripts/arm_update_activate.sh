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
    exec >>"$ARM_UPDATE_ACTIVE_DETAILS_LOG_PATH" 2>&1
    printf "%s: %s\n" "$(date '+%FT%T%z')" "Starting arm_update_activate.sh"
fi

# Returns successfully if the given path is a directory and is empty (except
# for maybe a 'lost+found' directory).
#
# $1: path of the directory to check
is_empty_directory() {
iedDirectory="$1"

    [ -d "$iedDirectory" ] || return 1

    test -z "$(find "$iedDirectory" -mindepth 1 -maxdepth 1 -not -name 'lost+found' -printf 'x' -quit)"
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
# $1: Path to the device file for the new root partition
# $2: Label for the new root partition
# $3: Image file (compressed tar file) with which to populate the new
#     partition. Trial and error is used to determine the compression algorithm
create_root_partition_or_die() {
crpodPart="$1"
crpodLabel="$2"
crpodImageFile="$3"

    crpodMountpoint=/mnt/root

    format_and_mount_root_partition_or_die "$crpodPart" "$crpodLabel" "$crpodMountpoint"

    for crpodDecompressFlag in -z -j -J; do
        # If a previous decompression failed then we might end up with some
        # stray content in our mountpoint. Reformat if that is the case.
        if ! is_empty_directory "$crpodMountpoint"; then
            format_and_mount_root_partition_or_die "$crpodPart" "$crpodLabel" "$crpodMountpoint"
        fi

        echo "Trying decompression flag \"${crpodDecompressFlag}\"..."
        if EXTRACT_UNSAFE_SYMLINKS=1 tar -x "$crpodDecompressFlag" -f "$crpodImageFile" -C "$crpodMountpoint"; then
            echo "Unarchiving with decompression flag \"${crpodDecompressFlag}\" succeeded"
            return 0
        fi
        echo "Unarchiving with decompression flag \"${crpodDecompressFlag}\" failed"
    done

    echo "Failed to find working decompression flag for image"
    exit 23
}


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

# For now, we have a temporary solution. At this stage we support single package/firmware updated. We expect one of 2 types of files :
# 1) Rootfs firmware update - in tar format
# 2) An opkg ipk file - wrapped in a tar format (needed to be extracted in order to get the actual ipk)
# Lets check if a single '*.ipk' file is inside the tar 
# If FIRMWARE is not a tar file we assume it's an IPK file for installation
SRC_IPK_PATH=/mnt/cache/opkg/src_ipk
if ! FIRMWARE_FILES=$(tar -tf "${FIRMWARE}")
then
    echo "tar -tf \"${FIRMWARE}\" failed!"
    exit 46
fi
if echo "${FIRMWARE_FILES}" | grep -q '.*\.ipk$' ; then
    if [ "$(echo "${FIRMWARE_FILES}" | wc -l)" -eq 1 ]; then
        # get the package name (the first token in the string when seperator is '_'). 
        # Comment : there are other options :
        # 1) Use opkg capabilities but I can't do that here due to some restrictions. 
        # 2) Look in the ipk control file. 
        IPK_FILE_NAME=${FIRMWARE_FILES}
        IPK_PKG_NAME=$(echo "${IPK_FILE_NAME}" | cut -d'_' -f1)
        
        # extract the actual ipk file into SRC_IPK_PATH
        if ! tar -xvf "${FIRMWARE}" -C ${SRC_IPK_PATH}
        then
            echo "tar -xvf \"${FIRMWARE}\" -C ${SRC_IPK_PATH} failed!"
            exit 41
        fi

        #remove package if installed
        if ! installed_pkgs=$(mbl-app-manager -l)
        then
            echo "mbl-app-manager -l failed!"
            exit 42
        fi
                
        is_package_installed=$(printf "%s" "$installed_pkgs" | awk -v "pkgname=${IPK_PKG_NAME}" '$1==pkgname { print $1 }')
        if [ -n "$is_package_installed" ]
        then
            if ! mbl-app-manager -r "${IPK_PKG_NAME}"
            then
                echo "mbl-app-manager -r \"${IPK_PKG_NAME} \" failed!"
                exit 44
            fi   
        fi
        
        #install package
        if ! mbl-app-manager -i "${SRC_IPK_PATH}/${IPK_FILE_NAME}"
        then
            echo "mbl-app-manager -i \"${SRC_IPK_PATH}/${IPK_FILE_NAME}\" failed!"
            exit 45
        fi
        
        #remove temporary ipk file
        rm -rf "${SRC_IPK_PATH:?}/${IPK_FILE_NAME:?}"
        
        # Flag that boot is not required
        touch /tmp/do_not_reboot
        exit 0
    else
        echo "Only a single package installation is supported!";
        exit 40
    fi
else
    # Remove old flag and if it fails, exit with error
    if ! rm -f /tmp/do_not_reboot; then
        echo "Failed to remove /tmp/do_not_reboot flag file";
        exit 27
    fi
fi

create_root_partition_or_die "$nextPartition" "$nextPartitionLabel" "$FIRMWARE"
ensure_not_mounted_or_die "$nextPartition"

ensure_mounted_or_die "${FLAGS}" /mnt/flags "$FLAGSFS_TYPE"

if ! cp "$HEADER" "/mnt/flags/${nextPartitionLabel}_header"; then
    echo "Failed to copy image header"
    exit 24
fi
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

ensure_not_mounted_or_die "$FLAGS"

exit 0
