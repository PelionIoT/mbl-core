#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest for testing MBL Firmware Update Manager."""

import importlib


class TestMblFirmwareUpdateManager:
    """MBL Firmware Update Manager main class."""

    def test_firmware_update_manager_mbl_subpackage(self):
        """
        Test that Firmware Update Manager is a subpackage of the "mbl" package.

        The Firmware Update Manager subpackage should be accessible via the
        "mbl" namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert (
            importlib.__import__(
                "mbl.firmware_update_manager.firmware_update_manager"
            )
            is not None
        )
