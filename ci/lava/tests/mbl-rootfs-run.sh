#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Find and select the device to talk to

test_stage=$1
image_name==${$2//wic.gz/tar.xz}
echo $image_name

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
    echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=RootFS_Update RESULT=fail>"
else

    mbl_command="mbl-cli -a $dut_address"

    active_partition=`$mbl_command shell 'lsblk --noheadings --output "MOUNTPOINT,LABEL"' | awk '$1=="/" {print $2}'`

    echo -n "Active Partition: "
    echo ${active_partition}


    if [ $test_stage = "UPDATE" ]
    then
        # At the start rootfs1 should be the active partition.
        if [ $active_partition = "rootfs1" ]
        then
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs1_selected RESULT=pass>"
        else
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs1_selected RESULT=fail>"
        fi

        $mbl_command shell 'echo "lava-"`hostname` > /config/user/hostname'

        apt-get update
        apt-get install -q -q --yes wget

        # Get the root filesystem image from the server.
        wget $image_name

        # Tar it up in the expected manner
        tar -cf payload.tar mbl-image-development-imx7s-warp-mbl.tar.xz '--transform=s/.*/rootfs.tar.xz/'

        # Now copy the tar file to the DUT
        $mbl_command put payload.tar /home/root

        # Now update the rootfs - the -s prevents the automatic reboot
        $mbl_command shell 'su -l -c "mbl-firmware-update-manager -i /home/root/payload.tar -v -s"'

        # Now reboot the board and get the result of the reboot command
        $mbl_command shell 'su -l -c "reboot || echo $?"'

        # Sleep to allow the reboot to happen
        sleep 60
    else
        # At the end rootfs2 should be the active partition.
        if [ $active_partition = "rootfs2" ]
        then
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs2_selected RESULT=pass>"
        else
            echo "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs2_selected RESULT=fail>"
        fi
    fi


fi

