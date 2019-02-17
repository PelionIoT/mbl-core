#!/bin/bash

# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Add non root user and call script creating resource manager sample application package.
#
# This script run inside the container; it adds non root user, switchs to it and calls
# script which creates resource manager sample application package.

SCRIPT_DIR=$(dirname ${0})

if ! getent passwd $USER_NAME > /dev/null ; then

  # Add group and user.
  groupadd -g $USER_ID $USER_NAME
  useradd -u $USER_ID -g $USER_ID $USER_NAME
  mkdir -p /home/$USER_NAME

fi

# Create package with a non root user
sudo -i -u $USER_NAME /bin/bash -c "${SCRIPT_DIR}/create_package_non_root.sh"

