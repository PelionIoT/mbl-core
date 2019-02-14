#!/bin/bash
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


# Creates resource manager sample application package.
#
# This script creates and run container, the container run make and
# creates the resource manager sample application package.

echo "*** Creating container:"
docker build cc-env/ -t resource_manager_app

echo "*** Running container and creating package inside the container:"
docker run -v $(pwd):/resource_manager_app -it resource_manager_app
