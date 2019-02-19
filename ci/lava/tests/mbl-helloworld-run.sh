#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Find and select the device to talk to

mbl-cli list > device_list
dut_address=$(grep "mbed-linux-os" device_list | cut -d":" -f3-)

cat device_list

rm device_list

if [ -z "$dut_address" ]
then
    echo "ERROR - mbl-cli failed to find MBL device"
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=fail>"
else

    mbl_command="mbl-cli -a $dut_address"


    # Initially attempt to cleanup any previous runs
    $mbl_command shell "rm /var/log/app/user-sample-app-package.log"

    # Now copy the package and ptyhon checker script to the DUT
    $mbl_command put /home/ubuntu/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar /home/app
    $mbl_command put ./ci/lava/dependencies/check_container.py /home/app

    # Now install the package - this should cause it to run
    $mbl_command shell "mbl-app-update-manager -i /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar"

    # Check it is executing as expected
    $mbl_command shell "python3 /home/app/check_container.py user-sample-app-package"

    # Extract the log file from the device. Nothe that the parsing of the log 
    # file could be done on the DUT but doing it this way tests the mbl-cli get
    # functionality.
    $mbl_command get /var/log/app/user-sample-app-package.log /home/ubuntu/

    # Echo it to the test run
    cat /home/ubuntu/user-sample-app-package.log

    # Count the number of times "Hello World" appears in the log. Anything other than 10 is a failure
    count_hello_world=$(grep -c "Hello, world" /home/ubuntu/user-sample-app-package.log)
    if [ "$count_hello_world" = 10 ]
    then
        echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=pass>"
    else
        echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=fail>"
    fi


    # Attempt to cleanup after the run
    $mbl_command shell "mbl-app-manager -r user-sample-app-package"
    $mbl_command shell "rm /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar"
    $mbl_command shell "rm /var/log/app/user-sample-app-package.log"

fi

