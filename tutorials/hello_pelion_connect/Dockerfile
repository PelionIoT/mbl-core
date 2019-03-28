# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

FROM resin/armhf-alpine:3.6

RUN [ "cross-build-start" ]

# Add hello-pelion-connect Python package to Docker image
ADD hello-pelion-connect /hello-pelion-connect

# Install Python3 and hello-pelion-connect application
RUN apk add --no-cache python3 py3-gobject3 && \
    python3 -m ensurepip && \
    rm -r /usr/lib/python*/ensurepip && \
    pip3 install --upgrade pip setuptools && \
    pip3 install pydbus && \ 
    pip3 install PyGObject && \ 
    pip3 install -U /hello-pelion-connect && \
    rm -r /root/.cache && \
    rm -r /.dockerenv && \
    rm -r /hello-pelion-connect && \
    rm -rf /var/cache/apk/*

# End cross-platform build
RUN [ "cross-build-end" ]
