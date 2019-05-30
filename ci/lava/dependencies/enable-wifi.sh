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

# Find the address of the first device found by the mbl-cli containing the
# pattern
mbl-cli list > device_list
dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3-)

# list the devices for debug
cat device_list

# Tidy up
rm device_list

mbl_command="mbl-cli -a $dut_address"


# Enable WiFi if the device under test needs it.

if [ "$device_type" =  "imx7s-warp-mbl" ] || [ "$device_type" =  "bcm2837-rpi-3-b-32" ] || [ "$device_type" =  "bcm2837-rpi-3-b-plus-32" ]
then

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
elif [ "$device_type" =  "imx7d-pico-mbl" ] || [ "$device_type" =  "imx8mmevk-mbl" ] 
then


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
            $mbl_command shell 'su -l -c "insmod /lib/modules/4.14.112-fslc+gf7f7fda/extra/qca9377.ko"'

            sleep 60

            # Enable WiFi
            $mbl_command shell 'connmanctl enable wifi'
            sleep 120

            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=pass>\n"

        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=fail>\n"
        fi

    fi
else
    # WiFi not needed/supported on the device so skip the test.
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable_wifi RESULT=skip>\n"
fi


