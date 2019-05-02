#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Enable WiFi

device_type=$1

# Enable WiFi if the device under test needs it.

if [ "$device_type" =  "imx7s-warp-mbl" ]
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
elif [ "$device_type" =  "imx7d-pico-mbl" ]
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
        apt-get install -q -q --yes wget

        wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-qca-2.0.3.bin
        if [ -f firmware-qca-2.0.3.bin ]; then
            chmod +x firmware-qca-2.0.3.bin

            $mbl_command put firmware-qca-2.0.3.bin /tmp

            $mbl_command shell 'ls -R  /lib/modules/'
            $mbl_command shell '/tmp/firmware-qca-2.0.3.bin --auto-accept'
            $mbl_command shell 'cp -v -r firmware-qca-2.0.3/1PJ_QCA9377-3_LEA_2.0/* /'

            $mbl_command shell 'ls -R  /lib/modules/'
            $mbl_command shell 'su -l -c "insmod /lib/modules/4.14.103-fslc\+g75401b0/extra/qca9377.ko"'
            sleep 120

            # Enable WiFi
            $mbl_command shell 'connmanctl enable wifi'
            sleep 120
            $mbl_command shell 'connmanctl scan wifi'
            $mbl_command shell 'connmanctl services'

            # Now reboot the board and get the result of the reboot command
            $mbl_command shell 'echo lava-"$(hostname)" > /config/user/hostname'
            $mbl_command shell 'su -l -c "reboot || echo $?"'
            sleep 40

            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=pass>\n"

        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=fail>\n"
        fi

    fi
else
    # WiFi not needed/supported on the device so skip the test.
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=skipped>\n"
fi


