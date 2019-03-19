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

    # Install the manifest-tool. This needs a newer version of pip, so get that
    # as well.
    pip3 -qqq install --upgrade pip
    pip3 -qqq install manifest-tool

    # Work out a unique certificate id - the first part of the path is the lava 
    # job number.
    certificate="$(pwd | cut -d"/" -f2)"
    api_key=ak_1MDE1ZTcxMTg4OGM4MDI0MjBhMDEwZDA1MDAwMDAwMDA0169725e06d512dc3bb0a403000000006nOrZ6DW1Ms5Gx5W3vU5O1zsFIyrrH

    # Run the manifest tool
    mkdir /tmp/update-resources
    cd /tmp/update-resources
    manifest-tool init -q -d arm.com -m dev-device

    # Deal with API keys.

    print "Try to save the api key.\n"
    mbl-cli save-api-key $api_key

    print "Check to see the key is installed correctly\n"
    key_found=grep -c $key ~/.mbl-store/user/config.json

    if [ $key_found -eq 1 ]
    then
        print "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=add-valid-key RESULT=pass>\n"
    else
        print "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=add-valid-key RESULT=fail>\n"
    fi


    mbl-cli save-api-key invalid_key >& /tmp/invalid_key
    invalid_rejected_ok=grep -c "API key not recognised by Pelion Device Management." ~/.mbl-store/user/config.json

    if [ $invalid_rejected_ok -eq 1 ]
    then
        print "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=reject-invalid-key RESULT=pass>\n"
    else
        print "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=reject-invalid-key RESULT=fail>\n"
    fi

    $mbl_command get-pelion-status

    $mbl_command provision-pelion -c $certificate anupdatecert -p /tmp/update-resources/update_default_resources.c

    $mbl_command get-pelion-status

    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=mbl-cli-provisioning RESULT=pass>"


fi

