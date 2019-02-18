#!/bin/bash

# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Create resource manager sample application package.
#
# This script creates the LWM2M resource manager sample application package.


SCRIPT_DIR=$(dirname "${0}")

LWM2M_RESOURCE_MANAGER_DIR="${SCRIPT_DIR}/bundle/rootfs/lwm2m_resource_manager"
IPK_IN_DIR="${SCRIPT_DIR}/ipk/in"
OPKG_BUILD="opkg-build -Z xz -g root -o root"

# create rootfs and lwm2m resource manager directory under it
mkdir -p "$LWM2M_RESOURCE_MANAGER_DIR"

cp "${SCRIPT_DIR}/src_bundle/config.json" "${SCRIPT_DIR}/bundle/config.json"

# copy source files under LWM2M_RESOURCE_MANAGER_DIR
# in order to copy python source files under LWM2M_RESOURCE_MANAGER_DIR
# directory uncomment next line
# cp -r ${SCRIPT_DIR}/src/* ${LWM2M_RESOURCE_MANAGER_DIR}

# create IPK_IN_DIR directory
mkdir -p "$IPK_IN_DIR"

cp -r "${SCRIPT_DIR}/bundle/config.json" "${IPK_IN_DIR}"
cp -r "${SCRIPT_DIR}/bundle/rootfs" "${IPK_IN_DIR}/rootfs"
cp -r "${SCRIPT_DIR}/src_opkg/CONTROL" "${IPK_IN_DIR}"

# create IPK
$OPKG_BUILD "$(realpath "${IPK_IN_DIR}")" "$(realpath "${SCRIPT_DIR}/ipk")"

