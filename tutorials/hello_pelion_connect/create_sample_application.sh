#!/bin/bash

# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# This script creates the hello_pelion_connect application package.
set -e

SCRIPT_DIR=$(dirname "${0}")

HELLO_PELION_CONNECT_DIR="${SCRIPT_DIR}"/bundle/rootfs/hello_pelion_connect
IPK_IN_DIR="${SCRIPT_DIR}"/ipk/in
OPKG_BUILD="opkg-build -Z xz -g root -o root"

# create rootfs and hello pelion connect directory under it
mkdir -p "$HELLO_PELION_CONNECT_DIR"

cp "${SCRIPT_DIR}"/src_bundle/config.json "${SCRIPT_DIR}"/bundle/config.json

# copy sources
cp -r "${SCRIPT_DIR}"/source/* "${HELLO_PELION_CONNECT_DIR}"
chmod +x "${SCRIPT_DIR}"/source/run_me.sh

# create IPK_IN_DIR directory
mkdir -p "$IPK_IN_DIR"

cp -r "${SCRIPT_DIR}"/bundle/config.json "${IPK_IN_DIR}"
cp -r "${SCRIPT_DIR}"/bundle/rootfs "${IPK_IN_DIR}"/rootfs
cp -r "${SCRIPT_DIR}"/src_opkg/CONTROL "${IPK_IN_DIR}"

# create IPK
$OPKG_BUILD "$(realpath "${IPK_IN_DIR}")" "$(realpath "${SCRIPT_DIR}/ipk")"

