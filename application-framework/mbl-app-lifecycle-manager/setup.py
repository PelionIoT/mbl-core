#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""setup.py file for MBL AppLifecycleManager package."""

import os
from setuptools import setup

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


setup(
    name="mbl_AppLifecycleManager",
    version="1",
    description="Mbed Linux OS Application Lifecycle Manager",
    long_description=read("README"),
    author="Arm Ltd.",
    author_email="",
    license="Apache-2.0",
    packages=["mbl.AppLifecycleManager"],
    zip_safe=False,
)
