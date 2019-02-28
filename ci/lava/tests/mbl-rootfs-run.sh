#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Find and select the device to talk to
apt install wget

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

    sleep 6000
    wget http://artifactory-proxy.mbed-linux.arm.com/artifactory/isg-mbed-linux/mbed-linux/mbl-master/mbl-master.1148/machine/imx7s-warp-mbl/images/mbl-image-development/images/mbl-image-development-imx7s-warp-mbl.tar.xz/mbl-image-development-imx7s-warp-mbl.tar.xz 
    sleep 6000

    # Now copy the package and ptyhon checker script to the DUT
    $mbl_command put mbl-image-development-imx7s-warp-mbl.tar.xz /home/root
    sleep 6000

    # Now install the package - this should cause it to run
    $mbl_command shell "mbl-app-firmware-manager -i /home/root/mbl-image-development-imx7s-warp-mbl.tar.xz"

    sleep 6000

    $mbl_command list

fi

