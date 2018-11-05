#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""setup.py file for Mbed Linux OS Application Manager package."""

import os
from setuptools import setup


def read(fname):
    """Utility function to read the README file."""
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


setup(
    name="mbl-app-manager",
    version="1",
    description="Mbed Linux OS Application Manager",
    long_description=read("README.md"),
    author="Arm Ltd.",
    author_email="",
    license="Apache-2.0",
    packages=["mbl.app_manager"],
    zip_safe=False,
)
