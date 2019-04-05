#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Run the Pelion provisioning tests and confirm they are functioning correctly

# If a parameter is passed in then assume it is a pattern to identify the
# target board, otherwise find something with "mbed-linux-os" in it.
if [ -z "$1" ]
then
    pattern="mbed-linux-os"
else
    pattern=$1
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
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-app-update  RESULT=fail>\n"
else


    $mbl_command get /var/log/mbl-cloud-client.log /tmp/mbl-cloud-client.log

    device_id=$(grep -i "device id"  /tmp/mbl-cloud-client.log | tail -1 | cut -d":" -f5)

    if [ -z "$device_id" ]
    then
        printf "ERROR - mbl-cli failed to find MBL device\n"
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-app-update RESULT=fail>\n"
    else
        # Now copy the python checker script to the DUT
        $mbl_command put ./ci/lava/dependencies/check_container.py /tmp

        cd /tmp/update-resources || exit
        cp /root/.mbed_cloud_config.json /tmp/update-resources
        manifest-tool update device --device-id "$device_id" --payload /tmp/user-sample-app-package_1.0_any.ipk.tar

        # Check it is executing as expected
        $mbl_command shell "python3 /tmp/check_container.py user-sample-app-package"

        # Extract the log file from the device. Note that the parsing of the log
        # file could be done on the DUT but doing it this way tests the mbl-cli get
        # functionality.
        $mbl_command get /var/log/app/user-sample-app-package.log /tmp/

        # Echo it to the test run
        cat /tmp/user-sample-app-package.log

        # Count the number of times "Hello World" appears in the log. Anything other than 10 is a failure
        count_hello_world=$(grep -c "Hello, world" /tmp/user-sample-app-package.log)
        if [ "$count_hello_world" = 10 ]
        then
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-app-update RESULT=pass>"
        else
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=pelion-app-update RESULT=fail>"
        fi

    fi

fi

