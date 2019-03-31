# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM resin/armhf-alpine:3.6

# We use cross-platform build to build ARM image on a 
# non-ARM development PC architecture 
RUN [ "cross-build-start" ]

# Add hello-pelion-connect Python package to Docker image
ADD hello-pelion-connect /hello-pelion-connect

# Install Python3 and hello-pelion-connect application
RUN apk add --no-cache python3 py3-gobject3 && \
    pip3 install --upgrade pip setuptools && \
    pip3 install pydbus PyGObject /hello-pelion-connect && \
    rm -r /root/.cache && \
    rm -r /.dockerenv && \
    rm -r /hello-pelion-connect && \
    rm -rf /var/cache/apk/*

# End cross-platform build
RUN [ "cross-build-end" ]
