#!/bin/bash -e


# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# This script is automating the steps found in
# https://os.mbed.com/docs/mbed-linux-os/master/develop-apps/hello-world-application.html

# This script must have 1 parameter passed, the arm_arch parameter
# $arm_arch contains the architecture type of the UUT which will determine
# the cross compiler to choose to build the user-sample-app-package binary.

print_result()
{
    err=$?
    if [ "$err" -eq 0 ]
    then
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-%s RESULT=pass>\n" "$testcase" "$teststep"
    else
        printf "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID=%s-%s RESULT=fail>\n" "$testcase" "$teststep"
    fi
}

trap print_result EXIT INT TERM

testcase="build-hello-world"

teststep="setup"

execdir="$(readlink -e "$(dirname "$0")")"

# Navigate to the sub-project directory
cd "${execdir}/../../../tutorials/helloworld"
print_result

# Work out the architecture type, based upon the device type.
# Possible values are: armv7, arm64

teststep="select-device"
case "$1" in
    "bcm2837-rpi-3-b-32" | "bcm2837-rpi-3-b-plus-32" | "imx7s-warp-mbl" | "imx7d-pico-mbl")
        arm_arch="armv7"
        ;;
    "imx8mmevk-mbl")
        arm_arch="arm64"
        ;;
    *)
        printf "Unsupported device type %s\n" "$1"
        exit 1
        ;;
esac
print_result

teststep="build-dockcross"
# Build dockcross cross compiler
docker build -t "linux-${arm_arch}:latest" "./cc-env/${arm_arch}"
print_result

# Run the image and capture into a script
teststep="run-dockcross"
docker run --rm "linux-${arm_arch}" > "./build-${arm_arch}"
print_result

# Make it executable.
chmod +x "./build-${arm_arch}"

# Build the hello world sample app
teststep="build-application"
"./build-${arm_arch}" make release
print_result

teststep="create-tarfile"
# Navigate to the location of the IPK
cd "${execdir}/../../../tutorials/helloworld/release/ipk"

# Get the image and put it into a tar file
tar -cvf user-sample-app-package_1.0_any.ipk.tar user-sample-app-package_1.0_any.ipk

# Copy the tar file to the /tmp directory this makes it available for
# subsequent test steps.

cp user-sample-app-package_1.0_any.ipk.tar /tmp/user-sample-app-package_1.0_any.ipk.tar

