#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Mbed Linux OS Application to connect to the Cloud Client setup.py."""

import os
from setuptools import setup


def read(fname):
    """Read the content of a file."""
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


setup(
    name="mbl-app-lifecycle",
    version="1",
    description="Mbed Linux OS Application connect to the Cloud Client",
    long_description=read("README.md"),
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.app_lifecycle"],
    zip_safe=False,
    entry_points={
        "console_scripts": ["mbl-app-lifecycle = mbl.app_lifecycle.cli:_main"]
    },
)
