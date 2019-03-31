#!/bin/bash

# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

# Creates hello_pelion_connect application package.
#
# To clean previously created artifacts call script with "clean" argument.

#abort on error
set -e

# Current directory absolute path
SCRIPT_DIR="$( realpath "$( dirname "${0}" )" )"

# clean
if [ "$1" = "clean" ]; then

    echo "Removing an existing artifacts..."
    rm -rf "${SCRIPT_DIR}/../release"
    exit 0
fi

# Build the hello-pelion-connect docker image
echo "Building the hello-pelion-connect docker image..."
docker build -t hello-pelion-connect "$SCRIPT_DIR/.."
echo "Exporting the docker image into a tar archive..."
mkdir "$SCRIPT_DIR/../release"
docker export -o "$SCRIPT_DIR/../release/hello-pelion-connect-docker-image.tar" "$(docker create hello-pelion-connect)"
echo "Extracting content from docker container..."
mkdir -p "$SCRIPT_DIR/../release/runtime-bundle-filesystem"
# Extract the filesystem with elevated permission for the operation to succeed
sudo tar -C "$SCRIPT_DIR/../release/runtime-bundle-filesystem" -xf "$SCRIPT_DIR/../release/hello-pelion-connect-docker-image.tar"
sudo chown -R "$(id -un)":"$(id -gn)" "release/runtime-bundle-filesystem"

echo "Creating IPK builder docker..."
docker build "${SCRIPT_DIR}/../cc-env/" -t hello_pelion_connect_ipk_builder

echo "Running IPK builder docker and creating IPK package..."
docker run --rm -e USER_NAME="$(whoami)" -e USER_ID="$UID" \
-v "$(realpath "${SCRIPT_DIR}/../")":/hello_pelion_connect \
-v /etc/localtime:/etc/localtime:ro \
-v /etc/timezone:/etc/timezone:ro \
-it hello_pelion_connect_ipk_builder

echo "IPK successfully created."
