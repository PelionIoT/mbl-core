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

    mbl_shell="mbl-cli -a $dut_address shell"

    # Only proceed if a device has been found

    if [ -z "$dut_address" ]
    then
        printf "ERROR - mbl-cli failed to find MBL device\n"
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=wired_wifi_test RESULT=fail>\n"
    else

        set +e
        $mbl_shell 'su -l -c "ifconfig"'
        $mbl_shell 'su -l -c "route"'
        $mbl_shell 'su -l -c "ping -c 1 -I eth0 8.8.8.8"'
        $mbl_shell 'su -l -c "ip link set eth0 down"'
        sleep 10
        $mbl_shell 'su -l -c "ifconfig"'
        $mbl_shell 'su -l -c "route"'
        $mbl_shell 'su -l -c "ping -c 1 -I wlan0 8.8.8.8"'
        $mbl_shell 'su -l -c "ping -c 1 -I eth0 8.8.8.8"'
        $mbl_shell 'su -l -c "ip link set eth0 up"'
        sleep 10
        $mbl_shell 'su -l -c "ifconfig"'
        $mbl_shell 'su -l -c "route"'
        $mbl_shell 'su -l -c "ping -c 1 -I wlan0 8.8.8.8"'
        $mbl_shell 'su -l -c "ping -c 1 -I eth0 8.8.8.8"'

        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=wired_wifi_test RESULT=pass>\n"
    fi
fi
