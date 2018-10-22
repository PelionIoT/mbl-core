#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""setup.py file for MBL AppLifecycleManager package."""

from setuptools import setup


setup(
    name="mbl_AppLifecycleManager",
    version="1",
    description="Mbed Linux OS Application Lifecycle Manager",
    long_description="This script implements mbl app lifecycle manager for starting, and stopping applications bundled as OCI containers.",
    author="ARM",
    author_email="",
    license="Apache-2.0",
    packages=["mbl.AppLifecycleManager"],
    zip_safe=False,
)
