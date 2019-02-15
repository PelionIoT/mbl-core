#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Find and select the device to talk to

set -x

mbl-cli list > device_list
dut_address=`grep "mbed-linux-os" device_list | cut -d":" -f3-`

cat device_list

rm device_list

mbl_command="mbl-cli -a $dut_address"


# Initially attempt to cleanup any previous runs
$mbl_command shell 'rm /var/log/app/user-sample-app-package.log'

# Now install the package - this should cause it to run
$mbl_command put /home/ubuntu/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar /home/app
$mbl_command shell 'mbl-app-update-manager -i /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar'

# Check it is there
$mbl_command shell 'mbl-app-manager -l'

# The app takes 20 seconds to run, so wait for 30
sleep 30

# Extract the log file from the device
$mbl_command get /var/log/app/user-sample-app-package.log /home/ubuntu/

# Echo it to the test run
cat /home/ubuntu/user-sample-app-package.log

# Count the number of times "Hello World" appears in the log. Anything other than 10 is a failure
if [ `grep -c "Hello, world" /home/ubuntu/user-sample-app-package.log` = 10 ]
then
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=pass>"
else
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=fail>"
fi


# Attempt to cleanup after the run
$mbl_command shell 'mbl-app-manager -r user-sample-app-package'
$mbl_command shell 'rm /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar'
$mbl_command shell 'rm /var/log/app/user-sample-app-package.log'


