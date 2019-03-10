#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""setup.py file for Mbed Linux OS Cloud Connect sample application package."""

import os
from setuptools import setup


def read(file_name):
    """Read the README file."""
    return open(os.path.join(os.path.dirname(__file__), file_name)).read()


setup(
    name="cloud-connect-sample-application",
    version="1",
    description="Mbed Linux OS Cloud Connect sample application package",
    long_description=read("README.md"),
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.cloud_connect_sample_application"],
    zip_safe=False,
    entry_points={
        "console_scripts": [
            "cloud_connect_sample_application = mbl.cloud_connect_sample_application.cli:_main"
        ]
    },
    include_package_data=True,
)

