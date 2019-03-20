#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Run the Pelion provisioning tests and confirm they are functioning correctly

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
    printf "ERROR - mbl-cli failed to find MBL device\n"
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=mbl-cli-provisioning RESULT=fail>\n"
else

    overall_result="pass"

    # Install the manifest-tool. This needs a newer version of pip, so get that
    # as well.
    pip3 -qqq install manifest-tool

    # Work out a unique certificate id - the first part of the path is the lava 
    # job number.
    certificate="$(pwd | cut -d"/" -f2)"

    # Run the manifest tool
    mkdir /tmp/update-resources
    cd /tmp/update-resources || exit
    manifest-tool init -q -d arm.com -m dev-device

    mbl-cli save-api-key invalid_key >& /tmp/invalid_key
    invalid_rejected_ok=$(grep -c "API key not recognised by Pelion Device Management." /tmp/invalid_key)

    if [ "$invalid_rejected_ok" -eq 1 ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=reject-invalid-key RESULT=pass>\n"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=reject-invalid-key RESULT=fail>\n"
        overall_result="fail"
        cat /tmp/invalid_key
    fi

    $mbl_command get-pelion-status >& /tmp/get-pelion-status

    pelion_not_ok=$(grep -c "Your device is not correctly configured for Pelion Device Management." /tmp/get-pelion-status)

    if [ "$pelion_not_ok" -eq 1 ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-not-configured RESULT=pass>\n"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-not-configured RESULT=fail>\n"
        overall_result="fail"
        cat /tmp/get-pelion-status
    fi

    $mbl_command provision-pelion -c "$certificate" anupdatecert -p /tmp/update-resources/update_default_resources.c >& /tmp/provision-pelion

    pelion_provisioned_ok=$(grep -c "Provisioning process completed without error." /tmp/provision-pelion)
    if [ "$pelion_provisioned_ok" -eq 1 ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-provisioned RESULT=pass>\n"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-provisioned RESULT=fail>\n"
        overall_result="fail"
        cat /tmp/provision-pelion
    fi

    $mbl_command get-pelion-status >& /tmp/get-pelion-status

    pelion_ok=$(grep -c "Device is configured correctly. You can connect to Pelion Cloud!" /tmp/get-pelion-status)

    if [ "$pelion_ok" -eq 1 ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-configured RESULT=pass>\n"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-configured RESULT=fail>\n"
        overall_result="fail"
        cat /tmp/get-pelion-status
    fi


    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=mbl-cli-provisioning RESULT=%s>\n" $overall_result


fi

