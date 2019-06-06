#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Enable WiFi

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

# Enable WiFi if the device under test needs it.
# Currently only Warp7 needs WiFi enabled.

if [ "$device_type" =  "imx7s-warp-mbl" ]
then

    # Find the address of the first device found by the mbl-cli containing the
    # pattern
    mbl-cli list > device_list
    dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3-)

    # list the devices for debug
    cat device_list

    # Tidy up
    rm device_list

    mbl_command="mbl-cli -a $dut_address"


    # Only proceed if a device has been found

    if [ -z "$dut_address" ]
    then
        printf "ERROR - mbl-cli failed to find MBL device\n"
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=fail>\n"
    else


        # Enable WiFi
        $mbl_command put /root/.wifi-access.config /config/user/connman/wifi-access.config


        # Enable WiFi
        $mbl_command shell 'connmanctl enable wifi'


        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=pass>\n"


    fi
else
    # WiFi not needed/supported on the device so skip the test.
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=skip>\n"
fi


