#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Parse the inputs
#
# The Python virtual environment will be activated if provided.
# The device under test can also be specified.

# Default to any device
pattern="mbed-linux-os"

while [ "$1" != "" ]; do
    case $1 in
        -v | --venv )   shift
                        # shellcheck source=/dev/null
                        source  "$1"/bin/activate
                        ;;
        -d | --dut )    shift
                        pattern=$1
                        ;;
        * )             echo "Invalid Parameter"
                        exit 1
    esac
    shift
done

# Find the address of the first device found by the mbl-cli containing the
# pattern
mbl-cli list > device_list
dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3-)

# list the devices for debug
cat device_list

# Tidy up
rm device_list

# Run the systemctl command via mbl-cli and emit the right LAVA signal
run_systemd_test()
{
    local systemctl_action="$1"
    local show_fail="$2"
    local systemctl_command="systemctl --no-pager $systemctl_action"
    local mbl_cli_shell="mbl-cli -a $dut_address shell"
    local mbl_cli_command="$mbl_cli_shell '$systemctl_command'"

    if eval "$mbl_cli_command"; then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s RESULT=pass>\n" "${systemctl_action// /_}"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s RESULT=fail>\n" "${systemctl_action// /_}"
        if [ "$show_fail" == "true" ]; then
            eval "$mbl_cli_shell 'systemctl --no-pager --failed'"
        fi
    fi
}


# Only proceed if a device has been found
if [ -z "$dut_address" ]
then
    printf "ERROR - mbl-cli failed to find MBL device\n"
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=systemd_tests RESULT=fail>\n"
else
    run_systemd_test "is-system-running" "true"

    actions=(
        "status boot.mount"
        "status config-factory.mount"
        "status config-user.mount"
        "status home.mount"
        "status mnt-flags.mount"
        "status scratch.mount"
        "status tmp.mount"
        "status var-log.mount"
        "status connman.service"
        "status dbus.service"
        "status mbl-app-update-manager.service"
        "status mbl-cloud-client.service"
        "status mbl-dbus-cloud.service"
        "status mbl-hostname.service"
        "status ofono.service"
        "status rngd.service"
        "status tee-supplicant.service"
        "status wpa_supplicant.service"
    )

    for action in "${actions[@]}"; do
        run_systemd_test "$action"
    done
fi

