#!/bin/bash

# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Create resource manager sample application package.
#
# This script run inside the container and creates the resource
# manager sample application package.


SCRIPT_DIR=$(dirname ${0})

ROOTFS_DIR="${SCRIPT_DIR}/bundle/rootfs"
CONTROL_DIR="${SCRIPT_DIR}/ipk/in/CONTROL"
OPKG_BUILD="opkg-build -Z "xz" -g root -o root"

# create rootfs directory
mkdir -p $ROOTFS_DIR

cp ${SCRIPT_DIR}/src_bundle/config.json ${SCRIPT_DIR}/bundle/config.json

# create CONTROL directory
mkdir -p $CONTROL_DIR

cp -r ${SCRIPT_DIR}/bundle/* ${SCRIPT_DIR}/ipk/in
cp -r ${SCRIPT_DIR}/src_opkg/CONTROL ${SCRIPT_DIR}/ipk/in

# create IPK
$OPKG_BUILD $(realpath ${SCRIPT_DIR}/ipk/in) $(realpath ${SCRIPT_DIR}/ipk)

