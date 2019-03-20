#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# monitor a running process and kill it if it doesn't return before the counter reaches 0
monitor_process() {
    pid=$1
    counter=$2
    while ps -p "$pid" > /dev/null
    do
        if [[ $counter -eq 0 ]]; then
            kill -9 "$pid"
        fi
        counter=$((counter-1))
        sleep 1
    done
}

# Run the avahi discovery test and confirm it is functioning correctly

# Find the address of the first device found by avahi-browse. This will only
# work correctly if there is a point to point connection between lxc and target.
avahi-browse -tr _ssh._tcp > device_list &
monitor_process $! 60 &
wait
mbl_device=$(grep "=" device_list | grep "mbed-linux-os" device_list)

# list the devices for debug
cat device_list

# Tidy up
rm device_list

# Check if a device has been found
if [ -z "$mbl_device" ]
then
    echo "ERROR - avahi-browse failed to find an MBL device"
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=AVAHI-DISCOVERY RESULT=fail>"
else
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=AVAHI-DISCOVERY RESULT=pass>"
fi
