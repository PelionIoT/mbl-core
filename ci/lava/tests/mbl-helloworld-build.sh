#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

cd tutorials/helloworld

docker build -t linux-armv7:latest ./cc-env/

docker run --rm linux-armv7 > ./build-armv7

chmod +x ./build-armv7

./build-armv7 make release

./build-armv7 make debug

cd debug/ipk/
tar -cvf user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar user-sample-app-package_1.0_armv7vet2hf-neon.ipk
cp user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar ~/user-sample-app-package_1.0_armv7vet2hf-neon.ipk.tar

