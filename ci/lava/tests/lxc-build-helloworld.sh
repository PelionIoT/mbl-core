#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# This script is automating the steps found in
# https://os.mbed.com/docs/mbed-linux-os/master/develop-apps/hello-world-application.html

# This script must have 1 parameter passed, the arm_arch parameter
# $arm_arch contains the architecture type of the UUT which will determine
# the cross compiler to choose to build the user-sample-app-package binary.

execdir="$(readlink -e "$(dirname "$0")")"

# Navigate to the sub-project directory
cd "${execdir}/../../../tutorials/helloworld" || exit

# Get the architecture type
arm_arch=$1

# Build dockcross cross compiler
docker build -t "linux-${arm_arch}:latest" "./cc-env/${arm_arch}"

# Run the image and capture into a script
docker run --rm "linux-${arm_arch}" > "./build-${arm_arch}"

# Make it executable.
chmod +x "./build-${arm_arch}"

# Build the hello world sample app
"./build-${arm_arch}" make release


# Navigate to the location of the IPK
cd "${execdir}/../../../tutorials/helloworld/release/ipk" || exit
# Get the image and put it into a tar file
tar -cvf user-sample-app-package_1.0_any.ipk.tar user-sample-app-package_1.0_any.ipk

# Copy the tar file to the home directory. This makes ii available for subsequent test steps.
cp user-sample-app-package_1.0_any.ipk.tar /tmp/user-sample-app-package_1.0_any.ipk.tar
