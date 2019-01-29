#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Sets up the mbl.firmware_update_header module."""

from setuptools import setup

setup(
    name="mbl-firmware-update-header",
    version="1",
    description="Mbed Linux OS firmware update HEADER blob utils library",
    author="Arm Ltd.",
    license="BSD-3-Clause",
    packages=["mbl.firmware_update_header"],
    zip_safe=False,
)
