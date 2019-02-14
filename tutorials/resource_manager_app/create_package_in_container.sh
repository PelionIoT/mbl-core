# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Create resource manager sample application package.
#
# This script run inside the container and creates the resource
# manager sample application package.


ROOTFS_DIR="bundle/rootfs"
CONTROL_DIR="ipk/in/CONTROL"
OPKG_BUILD="opkg-build -Z "xz" -g root -o root"


# create rootfs directory
if [ ! -e $ROOTFS_DIR ]; then
    mkdir -p $ROOTFS_DIR
fi

cp src_bundle/config.json bundle/config.json

# create CONTROL directory
if [ ! -e $CONTROL_DIR ]; then
    mkdir -p $CONTROL_DIR
fi

cp -r bundle/* ipk/in
cp -r src_opkg/CONTROL ipk/in

# create IPK
$OPKG_BUILD $(realpath ipk/in) $(realpath ipk)

