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

if [ -n "$ARM_UPDATE_ACTIVE_DETAILS_LOG_PATH" ]; then
    # Redirect stdout and stderr to the log file
    exec >>"$ARM_UPDATE_ACTIVE_DETAILS_LOG_PATH" 2>&1
    printf "%s: %s\n" "$(date '+%FT%T%z')" "Starting arm_update_active_details.sh"
fi

set -x

# header directory
HEADER_DIR=$(dirname "${HEADER}")

# header name (not prefix)
HEADER_NAME="header"

# location where the update client will find the header
HEADER_PATH="${HEADER_DIR}/${HEADER_NAME}.bin"

# Flag partition
FLAGS=$(get_device_for_label bootflags)
exit_on_error "$?"

ensure_mounted_or_die "$FLAGS" /mnt/flags "$FLAGSFS_TYPE"

# Boot Flags
if [ -e "$SAVED_HEADER_PATH" ]; then
    if ! cp "$SAVED_HEADER_PATH" "$HEADER_PATH"; then
        echo "Failed to copy image header"
        exit 22
    fi
else
    echo "Warning: No active firmware header found!"
    exit 23
fi

exit 0
