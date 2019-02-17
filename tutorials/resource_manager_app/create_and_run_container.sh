#!/bin/bash

# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


# Creates resource manager sample application package.
#
# This script creates and run container; resource manager sample application 
# package is created inside the container with startup command.

SCRIPT_DIR=$(dirname ${0})

# clean
if [ "$1" = 'clean' ]; then

    echo "*** Removing an existing container artifacts"
    rm -rf ${SCRIPT_DIR}/bundle
    rm -rf ${SCRIPT_DIR}/ipk

# create and run a container
else

    echo "*** Creating container:"
    docker build ${SCRIPT_DIR}/cc-env/ -t resource_manager_app

    echo "*** Running container and creating package inside the container:"
    docker run -e USER_NAME=$(whoami) -e USER_ID=$UID -v $(realpath $(dirname ${0})):/resource_manager_app -it resource_manager_app

fi
