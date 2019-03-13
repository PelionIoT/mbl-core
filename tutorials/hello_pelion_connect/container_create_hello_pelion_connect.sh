#!/bin/bash

# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


# Creates hello_pelion_connect application package.
#
# This script creates and run container on a PC; The purpose of the container
# is to invoke the build system and create the hello_pelion_connect application package.
# Inside the container all the necessary for creation application package tools
# are installed. Thus, the container provides a convenient framework for application
# creation and packaging. 
# To clean previously created artifacts call script with "clean" argument.
set -e

SCRIPT_DIR=$(dirname "${0}")

# clean
if [ "$1" = "clean" ]; then

    echo "*** Removing an existing container artifacts"
    rm -rf "${SCRIPT_DIR}/bundle"
    rm -rf "${SCRIPT_DIR}/ipk"

# create and run a container
else

    echo "*** Creating container:"
    docker build "${SCRIPT_DIR}/cc-env/" -t hello_pelion_connect_builder

    echo "*** Running container and creating package inside the container:"
    docker run --rm -e USER_NAME="$(whoami)" -e USER_ID="$UID" \
    -v "$(realpath "${SCRIPT_DIR}")":/hello_pelion_connect \
    -v /etc/localtime:/etc/localtime:ro \
    -v /etc/timezone:/etc/timezone:ro \
    -it hello_pelion_connect_builder

fi
