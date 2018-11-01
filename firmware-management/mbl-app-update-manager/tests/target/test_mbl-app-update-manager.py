#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest for testing MBL App Update Manager."""

import importlib


class TestMblAppUpdateManager:
    """MBL App Update Manager main class."""

    def test_app_update_manager_mbl_subpackage(self):
        """
        Test that App Update Manager is a subpackage of the "mbl" package.

        The App Update Manager subpackage should be accessible via the "mbl"
        namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.appupdatemgr") is not None
