#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Run the hello world test and confirm it is functioning correctly

# Parse the inputs
#
# The Python virtual environment will be cactivated if provided.
# The device under test can also be specified.

# Default to any device
pattern="mbed-linux-os"

while [ "$1" != "" ]; do
    case $1 in
        -v | --venv )   shift
                        # shellcheck source=/dev/null
                        source  "$1"/bin/activate
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

# Only proceed if a device has been found

if [ -z "$dut_address" ]
then
    echo "ERROR - mbl-cli failed to find MBL device"
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=helloworld-update RESULT=fail>"
else

    mbl_command="mbl-cli -a $dut_address"


    # Initially attempt to cleanup any previous runs
    $mbl_command shell "rm /var/log/app/user-sample-app-package.log"

    # Now copy the package and python checker script to the DUT
    $mbl_command put /tmp/user-sample-app-package_1.0_any.ipk.tar /tmp
    $mbl_command put ./ci/lava/dependencies/check_container.py /tmp

    # Now install the package - this should cause it to run
    $mbl_command shell "mbl-firmware-update-manager /tmp/user-sample-app-package_1.0_any.ipk.tar -v --keep --assume-no"

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
        echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=helloworld-update RESULT=pass>"
    else
        echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=helloworld-update RESULT=fail>"
    fi


    # Attempt to cleanup after the run
    $mbl_command shell "mbl-app-manager remove user-sample-app-package /home/app/user-sample-app-package"
    $mbl_command shell "rm /tmp/user-sample-app-package_1.0_any.ipk.tar"
    $mbl_command shell "rm /var/log/app/user-sample-app-package.log"

fi

