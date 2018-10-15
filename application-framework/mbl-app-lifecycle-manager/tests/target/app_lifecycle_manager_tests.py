#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""
Pytest for testing MBL App Lifecycle Manager.

In order to run pytest with prints:
cd /home/app/usr/bin
./python3 ./pytest.python3-pytest -s ./app_lifecycle_manager_tests.py

In order to run pytest without prints:
./python3 ./pytest.python3-pytest ./app_lifecycle_manager_tests.py
"""

import importlib


class TestMblAppLifecycleManager:
    """MBL App Lifecycle Manager main class."""

    def test_app_manager_mbl_subpackage(self):
        """
        Test that AppLifecycleManager is a subpackage of the "mbl" package.

        The AppLifecycleManager subpackage should be accessible via the "mbl"
        namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.util.find_spec("mbl.AppLifecycleManager") != None
