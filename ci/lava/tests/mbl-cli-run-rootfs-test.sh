#!/bin/bash

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


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
                        source  "$1"/activate
                        ;;
        -d | --dut )    shift
                        pattern=$1
                        ;;
        -s | --stage )  shift
                        test_stage=$1
                        ;;
        -i | --image )  shift
                        # $rootfs_image is the image name with the "wic.gz" replaced by "tar.xz"
                        rootfs_image=${1//wic.gz/tar.xz}
                        ;;
        -u | --update ) shift
                        pelion_update=$1
                        ;;
        * )             echo "Invalid Parameter"
                        exit 1
    esac
    shift
done

# Find the address of the first device found by the mbl-cli containing the
# pattern
mbl-cli list > device_list
cat device_list

dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3-)

rm device_list

if [ -z "$dut_address" ]
then
    printf "ERROR - mbl-cli failed to find MBL device\n"
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_RootFS_Update RESULT=fail>\n" "$pelion_update"
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
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs1_selected RESULT=pass>\n" "$pelion_update"
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs1_selected RESULT=fail>\n" "$pelion_update"
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
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs_download RESULT=pass>\n" "$pelion_update"
            # Tar it up in the expected manner. Take the url of the tar file
            # specified in rootfs_image and remove everything upto and
            # including the final "/"
            tar -cf /tmp/payload.tar "${rootfs_image##*/}" '--transform=s/.*/rootfs.tar.xz/'

            if [ "$pelion_update" = "PELION" ]; then

                $mbl_command get /var/log/mbl-cloud-client.log /tmp/mbl-cloud-client.log

                device_id=$(grep -i "device id"  /tmp/mbl-cloud-client.log | tail -1 | cut -d":" -f5 | cut -c2-)

                if [ -z "$device_id" ]
                then
                    printf "ERROR - mbl-cli failed to find MBL device in the mbl-cloud-client.log\n"
                    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_pelion-app-update RESULT=fail>\n" "$pelion_update"
                else

                    cd /tmp/update-resources || exit
                    cp /root/.mbed_cloud_config.json /tmp/update-resources
                    manifest-tool update device --device-id "$device_id" --payload /tmp/payload.tar
                fi

            else


                # Now copy the tar file to the DUT
                $mbl_command put /tmp/payload.tar /scratch

                # Now update the rootfs - the -s prevents the automatic reboot
                $mbl_command shell 'su -l -c "mbl-firmware-update-manager /scratch/payload.tar -v --keep --assume-no"'

                # Now reboot the board and get the result of the reboot command
                $mbl_command shell 'su -l -c "reboot || echo $?"'

                # Sleep to allow the reboot to happen. This is nasty but is long enough
                # for the DUT to shut down but not long enough for it to fully restart.
                sleep 40
            fi
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs_download RESULT=fail>\n" "$pelion_update"
        fi

    else # The POST_CHECK
        # At the end rootfs2 should be the active partition.
        if [ "$active_partition" = "rootfs2" ]
        then
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs2_selected RESULT=pass>\n" "$pelion_update"
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs2_selected RESULT=fail>\n" "$pelion_update"
        fi
    fi


fi

