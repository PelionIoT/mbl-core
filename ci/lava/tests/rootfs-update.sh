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
                        source  "$1"/bin/activate
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
        -r | --rootfs ) shift
                        rootfs=$1
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

dut_address=$(grep "$pattern" device_list | head -1 | cut -d":" -f3- | tr -d ' ')

rm device_list

if [ -z "$dut_address" ]
then
    printf "ERROR - mbl-cli failed to find MBL device\n"
    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s_rootfs-update RESULT=fail>\n" "$pelion_update"
else

    mbl_command="mbl-cli -a $dut_address"

    # Work out current active partition.
    active_partition=$($mbl_command shell 'lsblk --noheadings --output "MOUNTPOINT,LABEL"' | grep "^/ " | awk '{print $2}')

    printf "Active Partition: %s\n" "$active_partition"

    # We check if the rootfs pass as argument is the same of the active rootfs
    if [ "$active_partition" = "$rootfs" ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-%s-selected RESULT=pass>\n" "$pelion_update" "$rootfs"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-%s-selected RESULT=fail>\n" "$pelion_update" "$rootfs"
    fi

    if [ "$test_stage" = "UPDATE" ]
    then
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
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-rootfs-download RESULT=pass>\n" "$pelion_update"
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
                    printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-rootfs-update RESULT=fail>\n" "$pelion_update"
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
                # $mbl_command shell 'su -l -c "reboot || echo $?"'
                # mbl-cli doesn't close the connection properly when we run a reboot: IOTMBL-2389
                # Until we fix it, we reboot the board sending the command over ssh with the appropriate
                # options ServerAliveInterval=10 -o ServerAliveCountMax=3
                ssh_options=("-o" "StrictHostKeyChecking=no" "-o" "ServerAliveInterval=5" "-o" "ServerAliveCountMax=1")
                # Check if the address is IPv4. If not, assume it is IPv6.
                # ssh command needs to be piped with true because when the connection hangs
                # it returns an error code which we don't need to check.
                printf "Rebooting device...\n"
                if [[ $dut_address =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
                    ssh "${ssh_options[@]}" "$dut_address" 'su -l -c "reboot || echo $?"' | true
                else
                    ssh "${ssh_options[@]}" -6 "$dut_address" 'su -l -c "reboot || echo $?"' | true
                fi
            fi
        else
            printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-rootfs-download RESULT=fail>\n" "$pelion_update"
        fi
    fi
fi
