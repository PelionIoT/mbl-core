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

if [ "$device_type" = "imx7s-warp-mbl" ] || \
   [ "$device_type" = "bcm2837-rpi-3-b-32" ] || \
   [ "$device_type" = "bcm2837-rpi-3-b-plus-32" ]
then


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

elif [ "$device_type" = "imx7d-pico-mbl" ] || \
     [ "$device_type" = "imx8mmevk-mbl" ]  || \
     [ "$device_type" = "imx6ul-pico-mbl" ]
then


    # Only proceed if a device has been found

    if [ -z "$dut_address" ]
    then
        printf "ERROR - mbl-cli failed to find MBL device\n"
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=fail>\n"
    else
        # Blacklist the p2p interface
        $mbl_command shell 'sed -i "s/#NetworkInterfaceBlacklist=/NetworkInterfaceBlacklist=p2p/" /config/user/connman/main.conf'

        # Force reload of the file
        $mbl_command shell 'su -l -c "systemctl daemon-reload"'

        # Restart connman
        $mbl_command shell 'su -l -c "systemctl restart connman"'

        # Sleep to allow the restart to take effect
        sleep 20

        # Enable WiFi
        $mbl_command put /root/.wifi-access.config /config/user/connman/wifi-access.config

        # Check to see if the firmware is already loaded.
        if [ "$($mbl_command shell 'lsmod' | grep -c qca9377)" = 0 ]
        then

            # Not loaded so fetch the firmware and install it.

            # Enable WiFi
            apt-get install -q -q --yes wget


            wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-qca-2.0.3.bin
            if [ -f firmware-qca-2.0.3.bin ]; then
                chmod +x firmware-qca-2.0.3.bin


                $mbl_command put firmware-qca-2.0.3.bin /tmp

                $mbl_command shell '/tmp/firmware-qca-2.0.3.bin --auto-accept'
                $mbl_command shell 'cp -v -r firmware-qca-2.0.3/1PJ_QCA9377-3_LEA_2.0/* /'

                # Need to use a wildcard to locate the module because it is not fixed
                $mbl_command shell "su -l -c 'insmod /lib/modules/*/extra/qca9377.ko'"

                # Wait for the module to be loaded.
                sleep 60

                printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=pass>\n"

            else
                printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=fail>\n"
            fi
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=pass>\n"
        fi

        # Enable WiFi
        $mbl_command shell 'connmanctl enable wifi'
    fi
else
    # WiFi not needed/supported on the device so skip the test.
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=enable-wifi RESULT=skip>\n"
fi


