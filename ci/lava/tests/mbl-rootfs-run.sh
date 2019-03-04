#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Find and select the device to talk to

mbl-cli list > device_list
cat device_list

dut_address=$(grep "mbed-linux-os" device_list | cut -d":" -f3-)

if [ -z "$dut_address" ]
then
    echo "ERROR - mbl-cli failed to find MBL device"
    echo "ERROR - mbl-cli retry with linux"
    dut_address=$(grep "linux" device_list | cut -d":" -f3-)
fi


rm device_list

if [ -z "$dut_address" ]
then
    echo "ERROR - mbl-cli failed to find MBL device"
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=HelloWorld RESULT=fail>"
else

    mbl_command="mbl-cli -a $dut_address"

    $mbl_command shell 'lsblk --noheadings --output "MOUNTPOINT,LABEL"' | awk '$1=="/" {print $2}'

    wget http://artifactory-proxy.mbed-linux.arm.com/artifactory/isg-mbed-linux/mbed-linux/mbl-master/mbl-master.1148/machine/imx7s-warp-mbl/images/mbl-image-development/images/mbl-image-development-imx7s-warp-mbl.tar.xz/mbl-image-development-imx7s-warp-mbl.tar.xz 

    tar -cf payload.tar mbl-image-development-imx7s-warp-mbl.tar.xz '--transform=s/.*/rootfs.tar.xz/'

    # Now copy the package and ptyhon checker script to the DUT
    $mbl_command put payload.tar /home/root

    # Now install the package - this should cause it to run
    #$mbl_command shell '/bin/sh -c "PATH=${PATH}:/usr/sbin;/sbin;"mbl-firmware-update-manager -i /home/root/payload.tar  -v"'
    $mbl_command shell 'su -l -c "mbl-firmware-update-manager -i /home/root/payload.tar  -v"'
    sleep 60

fi

