#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# This script is automating the steps found in
# https://os.mbed.com/docs/mbed-linux-os/v0.5/getting-started/tutorial-user-application.html

cd tutorials/helloworld || exit

# Build dockcross cross compiler
docker build -t linux-armv7:latest ./cc-env/

# Run the image and capture into a script
docker run --rm linux-armv7 > ./build-armv7

# Make it executable.
chmod +x ./build-armv7

# Build the hello world sample app
./build-armv7 make release


# Get the image and put it into a tar file
cd release/ipk/ || exit
tar -cvf user-sample-app-package_1.0_armv7vet2hf-any.ipk.tar user-sample-app-package_1.0_armv7vet2hf-any.ipk

# Copy the tar file to the home directory. This makes ii available for subsequent test steps.
cp user-sample-app-package_1.0_armv7vet2hf-any.ipk.tar /home/ubuntu/user-sample-app-package_1.0_armv7vet2hf-any.ipk.tar

