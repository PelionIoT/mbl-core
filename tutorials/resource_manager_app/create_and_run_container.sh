#!/bin/bash

# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


# Creates resource manager sample application package.
#
# This script creates and run container; resource manager sample application 
# package is created inside the container with startup command


#clean
if [ "$1" = 'clean' ]; then
    rm -rf bundle
    rm -rf ipk
    
else

    echo "*** Creating container:"
    docker build cc-env/ -t resource_manager_app

    echo "*** Running container and creating package inside the container:"
    docker run -v $(pwd):/resource_manager_app -it resource_manager_app

fi
