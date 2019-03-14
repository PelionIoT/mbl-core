#!/bin/bash

# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# This script run inside the OCI container on a device. It runs Hello Pelion 
# Connect application in the Python virtual environment.

set -e

APP_NAME="hello_pelion_connect"
DEST_FOLDER="/tmp"

# The application should be installed into a writable folder using the Python
# virtual environment. In the OCI container the /tmp/ folder has a write 
# permissions.
echo "Coping hello pelion connect application code to ${DEST_FOLDER}..."  
mkdir ${DEST_FOLDER}/${APP_NAME}

cp -r /${APP_NAME}/* ${DEST_FOLDER}/${APP_NAME}/

cd ${DEST_FOLDER}/${APP_NAME}

echo "Create and activate virtual environment..."  
python3 -m venv my_venv

# shellcheck disable=SC1091
source ./my_venv/bin/activate

echo "Install requirements..."  
pip install --upgrade pip

pip install -r requirements.txt

echo "Install the application..."  
pip install . --upgrade

echo "Run the application..." 
${APP_NAME}

echo "Deactivate and clean..."
deactivate
rm -rf ./my_venv/
