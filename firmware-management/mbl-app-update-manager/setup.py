#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""setup.py file for Mbed Linux OS Application Update Manager package."""

import os
from setuptools import setup


def read(fname):
    """Utility function to read the README file."""
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


scripts = ["bin/mbl-app-update-manager-daemon"]

setup(
    name="mbl-app-update-manager",
    version="1",
    description="Mbed Linux OS Application Update Manager",
    long_description=read("README.md"),
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.app_update_manager"],
    zip_safe=False,
    entry_points={
        "console_scripts": [
            "mbl-app-update-manager = mbl.app_update_manager.cli:_main"
        ]
    },
    scripts=scripts,
)
