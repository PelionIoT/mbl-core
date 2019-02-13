#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
set +x

# Find and select the device to talk to
echo "1" | mbl-cli select

# Initially attempt to cleanup any previous runs
mbl-cli shell 'mbl-app-manager -r user-sample-app-package'
mbl-cli shell 'rm /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk'
mbl-cli shell 'rm /var/log/app/user-sample-app-package.log'

# Now install the package - this should cause it to run
mbl-cli put /home/ubuntu/user-sample-app-package_1.0_armv7vet2hf-neon.ipk /home/app
mbl-cli shell 'mbl-app-manager -i /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk'

# Check it is there
mbl-cli shell 'mbl-app-manager -l'

# The app takes 20 seconds to run, so wait for 30
sleep 30

# Extract the log file from the device
mbl-cli get /var/log/app/user-sample-app-package.log /home/ubuntu/

# Echo it to the test run
cat /home/ubuntu/user-sample-app-package.log

# Attempt to cleanup anfter the run
mbl-cli shell 'mbl-app-manager -r user-sample-app-package'
mbl-cli shell 'rm /home/app/user-sample-app-package_1.0_armv7vet2hf-neon.ipk'
mbl-cli shell 'rm /var/log/app/user-sample-app-package.log'


