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

update_apps_or_die "$1"
exit 0
