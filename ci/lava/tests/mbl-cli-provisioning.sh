#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Run the hello world test and confirm it is functioning correctly

# Find the address of the first device found by the mbl-cli. This will only
# work correctly if there is a point to point connection between lxc and target.
mbl-cli list > device_list
dut_address=$(grep "mbed-linux-os" device_list | head -1 | cut -d":" -f3-)

# list the devices for debug
cat device_list

# Tidy up
rm device_list

mbl_command="mbl-cli -a $dut_address"


# Only proceed if a device has been found

if [ -z "$dut_address" ]
then
    echo "ERROR - mbl-cli failed to find MBL device"
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=mbl-cli-provisioning RESULT=fail>"
else

set -x

    pip install --upgrade pip

    pip3 install manifest-tool

    certificate=$(pwd) | cut -d"/" -f2

    mkdir /tmp/update-resources
    cd /tmp/update-resources

    manifest-tool init -q -d arm.com -m dev-device

    mbl-cli save-api-key ak_1MDE1ZTcxMTg4OGM4MDI0MjBhMDEwZDA1MDAwMDAwMDA0169725e06d512dc3bb0a403000000006nOrZ6DW1Ms5Gx5W3vU5O1zsFIyrrHAv

    cat ~/.mbl-store/user/config.json

    mbl-cli save-api-key ahdskjhdakhkjas

    $mbl_command get-pelion-status

    $mbl_command provision-pelion -c $certificate anupdatecert -p /tmp/update-resources/update_default_resources.c

    $mbl_command get-pelion-status

    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=mbl-cli-provisioning RESULT=pass>"


fi

