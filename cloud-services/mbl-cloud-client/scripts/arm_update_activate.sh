#!/bin/sh
# ----------------------------------------------------------------------------
# Copyright (c) 2016-2019 Arm Limited and Contributors. All rights reserved.
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

# Note - exit codes 1-20 come from arm_update_common.sh.

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


# Save the update header to a standard place
#
# $1: Header to save
save_header_or_die() {
shodHeader="$1"
    if ! cp "$shodHeader" "$SAVED_HEADER_PATH"; then
        echo "Failed to copy image header"
        exit 21
    fi
    sync
}


# Remove temporary files.
# To be called from exit handler
clean_up() {
    rm -rf "$PAYLOAD_TMP_DIR"
}

# Set up the temporary directory used for storing component payloads
# Exits on errors
set_up_payload_tmp_dir_or_die() {
    if ! rm -rf "$PAYLOAD_TMP_DIR"; then
        printf "Failed to remove previously existing tmp dir \"%s\"\n" "$PAYLOAD_TMP_DIR"
        exit 22
    fi
    if ! mkdir -p "$PAYLOAD_TMP_DIR"; then
        printf "Failed to create tmp dir \"%s\"\n" "$PAYLOAD_TMP_DIR"
        exit 23
    fi
}

# ------------------------------------------------------------------------------
# Main code starts here
# ------------------------------------------------------------------------------

if ! FIRMWARE_PATH=$(realpath "$FIRMWARE"); then
    printf "Failed to resolve path to payload file \"%s\"\n" "$FIRMWARE"
    exit 24
fi

# Set up the temporary directory used by the installer scripts (which are called by
# swupdate). The installers depend on this directory being present. We also clean
# up the directory when the script exits.
set_up_payload_tmp_dir_or_die
trap clean_up EXIT

# We only have to reboot during the update if there's an update for a component
# that requires a reboot, so set the do_not_reboot flag here and let it be
# deleted if a reboot is required. It is the responsibilty of the installer scripts
# to delete this file if a reboot is required.
touch "${TMP_DIR}/do_not_reboot"

# Call swupdate and pass it the payload cpio archive. swupdate will then delegate
# to our installer scripts to perform the update.
if ! swupdate -i "$FIRMWARE_PATH"; then
    printf "Call to swupdate failed.\n"
    exit 25
fi

save_header_or_die "$HEADER"

if [ -e "${TMP_DIR}/do_not_reboot" ]; then
    # None of the component installers want a full reboot, but:
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
fi
exit 0
