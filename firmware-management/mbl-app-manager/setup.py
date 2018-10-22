#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""setup.py file for MBL AppManager package."""

from setuptools import setup


setup(
    name="mbl_AppManager",
    version="1",
    description="Mbed Linux OS Application Manager",
    long_description="This script implements mbl app manager for installing, removing and listing installed apps using opkg.",
    author="ARM",
    author_email="",
    license="Apache-2.0",
    packages=["mbl.AppManager"],
    zip_safe=False,
)
