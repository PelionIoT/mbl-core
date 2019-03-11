#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


# This script must have 2 parameters passed, the test stage (UPDATE or
# POST_CHECK) and the image_url of the wic file.
#
# Set the local variables:
# $test_stage contaons the stage
# $rootfs_image is the image name with the "wic.gz" replaced by "tar.xz"
#

test_stage=$1

# Perform global substitution
rootfs_image=${2//wic.gz/tar.xz}

# Find and select the device to talk to

mbl-cli list > device_list
cat device_list

dut_address=$(grep "mbed-linux-os" device_list | cut -d":" -f3-)

rm device_list

if [ -z "$dut_address" ]
then
    printf "ERROR - mbl-cli failed to find MBL device\n"
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=RootFS_Update RESULT=fail>\n"
else

    mbl_command="mbl-cli -a $dut_address"

    # Work out current active partition. 
    active_partition=$($mbl_command shell 'lsblk --noheadings --output "MOUNTPOINT,LABEL"' | awk '$1=="/" {print $2}')

    printf "Active Partition: %s\n" "$active_partition"


    if [ "$test_stage" = "UPDATE" ]
    then
        # At the start rootfs1 should be the active partition, since the board
        # should be cleanly flashed.
        if [ "$active_partition" = "rootfs1" ]
        then
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs1_selected RESULT=pass>\n"
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs1_selected RESULT=fail>\n"
        fi

        # Update the hostname of the DUT to contain "lava-". This is so the
        # test stage after the reboot will expect a different prompt.


        # The single quotes upsets shellcheck, however we need to send the
        # command as specified to the DUT. Otherwise the hostname is expanded
        # on the host rather than on the DUT.
        # shellcheck disable=SC2016
        $mbl_command shell 'echo lava-"$(hostname)" > /config/user/hostname'

        # Install wget
        apt-get update
        apt-get install -q -q --yes wget

        # Get the root filesystem image from the server.
        wget "$rootfs_image"
        retVal=$?
        if [ "$retVal" -ne 0 ]; then
            # The wget failed. This could be because the server in the main farm has been refused the connection.
            # https://confluence.arm.com/display/mbedlinux/LAVA+and+Artifactory#LAVAandArtifactory-Ouluinstance suggests
            # this can be worked around by mapping the name to IP address in /etc/hosts so try that.
            printf "wget failed. Trying alternative method on adding mapping between 192.168.130.43  artifactory-proxy.mbed-linux.arm.com into /etc/hosts.\n"
            printf "192.168.130.43  artifactory-proxy.mbed-linux.arm.com" >> /etc/hosts
            wget "$rootfs_image"
            retVal=$?
            if [ "$retVal" -ne 0 ]; then
                printf "Alternative method also failed.\n"
            fi
        fi

        if [ $retVal -eq 0 ]; then
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs_download RESULT=pass>\n"
            # Tar it up in the expected manner
            tar -cf payload.tar ${rootfs_image##*/} '--transform=s/.*/rootfs.tar.xz/'

            # Now copy the tar file to the DUT
            $mbl_command put payload.tar /scratch

            # Now update the rootfs - the -s prevents the automatic reboot
            $mbl_command shell 'su -l -c "mbl-firmware-update-manager -i /scratch/payload.tar -v -s"'

            # Now reboot the board and get the result of the reboot command
            $mbl_command shell 'su -l -c "reboot || echo $?"'

            # Sleep to allow the reboot to happen. This is nasty but is long enough
            # for the DUT to shut down but not long enough for it to fully restart.
            sleep 40
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs_download RESULT=fail>\n"
        fi

    else # The POST_CHECK
        # At the end rootfs2 should be the active partition.
        if [ "$active_partition" = "rootfs2" ]
        then
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs2_selected RESULT=pass>\n"
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=rootfs2_selected RESULT=fail>\n"
        fi
    fi


fi

