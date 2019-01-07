#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""setup.py file for Mbed Linux OS open ports checker package."""

import os
from setuptools import setup


def read(file_name):
    """Utility function to read the README file."""
    return open(os.path.join(os.path.dirname(__file__), file_name)).read()


setup(
    name="open-ports-checker",
    version="1",
    description="Mbed Linux OS open ports checker package",
    long_description=read("README.md"),
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.open_ports_checker"],
    zip_safe=False,
    entry_points={
        "console_scripts": [
            "open_ports_checker = mbl.open_ports_checker.cli:_main"
        ]
    },
    include_package_data=True
)
