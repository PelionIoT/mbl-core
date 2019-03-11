#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""setup.py file for Mbed Linux OS Hello Pelion Connect application package."""

import os
from setuptools import setup


def read(file_name):
    """Read the README file."""
    return open(os.path.join(os.path.dirname(__file__), file_name)).read()


setup(
    name="hello-pelion-connect",
    version="1",
    description="Mbed Linux OS Hello Pelion Connect application package",
    long_description=read("README.md"),
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.hello_pelion_connect"],
    zip_safe=False,
    entry_points={
        "console_scripts": [
            "hello_pelion_connect = mbl.hello_pelion_connect.cli:_main"
        ]
    },
    include_package_data=True,
)
