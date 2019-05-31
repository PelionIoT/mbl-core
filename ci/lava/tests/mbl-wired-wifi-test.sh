#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Run the Wifi and Wired tests

# Default to any device

pattern="mbed-linux-os"

while [ "$1" != "" ]; do
    case $1 in
        -v | --venv )   shift
                        # shellcheck source=/dev/null
                        source  "$1"/bin/activate
                        ;;
        -t | --dev )    shift
                        device_type=$1
                        ;;
        -d | --dut )    shift
                        pattern=$1
                        ;;
        * )             echo "Invalid Parameter"
                        exit 1
    esac
    shift
done

run_ping_test()
{
    local test_command="ping -c 1 -I $1 $2"
    local mbl_cli_command="$mbl_cli_shell '$test_command'"


    if eval "$mbl_cli_command"; then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s RESULT=pass>\n" "${test_command// /_}"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s RESULT=fail>\n" "${test_command// /_}"
    fi
}

disable_interface()
{
    local test_command="su -l -c \"ip link set $1 down\""
    local mbl_cli_command="$mbl_cli_shell '$test_command'"
    eval $mbl_cli_command
}

enable_interface()
{
    local test_command="su -l -c \"ip link set $1 up\""
    local mbl_cli_command="$mbl_cli_shell '$test_command'"
    eval $mbl_cli_command
}

if [ "$device_type" =  "imx7s-warp-mbl" ]
then

    # Wired not supported
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=wired_wifi_test RESULT=skip>\n"

else

    # Find the address of the first device found by the mbl-cli containing the
    # pattern
    mbl-cli list > device_list
    dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3-)

    # list the devices for debug
    cat device_list

    # Tidy up
    rm device_list

    mbl_cli_shell="mbl-cli -a $dut_address shell"

    # Only proceed if a device has been found

    if [ -z "$dut_address" ]
    then
        printf "ERROR - mbl-cli failed to find MBL device\n"
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=wired_wifi_test RESULT=fail>\n"
    else

        set +e
        $mbl_cli_shell 'su -l -c "ifconfig"'
        $mbl_cli_shell 'su -l -c "route"'
        run_ping_test "eth0" "8.8.8.8"
        run_ping_test "eth0" "www.google.com"

        disable_interface "eth0"

        sleep 10
        $mbl_cli_shell 'su -l -c "ifconfig"'
        $mbl_cli_shell 'su -l -c "route"'
        run_ping_test "wlan0" "8.8.8.8"
        run_ping_test "wlan0" "www.google.com"
        run_ping_test "eth0" "8.8.8.8"
        run_ping_test "eth0" "www.google.com"
        enable_interface "eth0"

        sleep 10
        $mbl_cli_shell 'su -l -c "ifconfig"'
        $mbl_cli_shell 'su -l -c "route"'
        run_ping_test "wlan0" "8.8.8.8"
        run_ping_test "wlan0" "www.google.com"
        run_ping_test "eth0" "8.8.8.8"
        run_ping_test "eth0" "www.google.com"

        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=wired_wifi_test RESULT=pass>\n"
    fi
fi
