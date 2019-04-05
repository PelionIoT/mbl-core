#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Enable WiFi

device_type=$1

if [ $device_type =  "imx7s-warp-mbl" ]
then

    # If a second parameter is passed in then assume it is a pattern to 
    # identify the target board, otherwise find something with "mbed-linux-os"
    # in it.
    if [ -z "$2" ]
    then
        pattern="mbed-linux-os"
    else
        pattern=$2
    fi

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
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=fail>\n"
    else


        # Enable WiFi
        $mbl_command put /root/.wifi-access.config /config/user/connman/wifi-access.config


        # Enable WiFi
        $mbl_command shell 'connmanctl enable wifi'


        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=pass>\n"


    fi
else
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=skipped>\n"
fi


