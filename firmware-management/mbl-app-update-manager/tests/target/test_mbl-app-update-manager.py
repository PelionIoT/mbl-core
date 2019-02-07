#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing MBL App Update Manager."""

import importlib


class TestMblAppUpdateManager:
    """MBL App Update Manager main class."""

    def test_app_update_manager_mbl_subpackage(self):
        """
        Test that all modules can be imported as part of the `mbl` namespace
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.app_update_manager.cli") is not None
        assert (
            importlib.__import__("mbl.app_update_manager.manager") is not None
        )
        assert importlib.__import__("mbl.app_update_manager.utils") is not None
