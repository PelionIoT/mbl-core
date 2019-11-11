#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to install and run cellular connectivity tests on the DUT.

This pytest script consists of a test_setup function that installs the
necessary files for testing onto the DUT.

The test_component function runs pytest on the DUT via the mbl-cli on the
copied files.

"""
import os
import pytest
import re


class TestCellularAndWifiHost:
    """Class to encapsulate the testing of cellular connctivity on a DUT."""

    def test_setup(self, dut_addr, execute_helper, dut_tutorials_dir):
        """Copy the necessary files to DUT."""
        execute_helper.send_mbl_cli_command(
            ["put", "-r", "./", "{}/mbl-core".format(dut_tutorials_dir)],
            dut_addr,
        )

    def test_cellular(
        self, dut_addr, execute_helper, venv, dut_tutorials_dir, device
    ):
        """Perform the test on the DUT via the mbl-cli."""
        if (
            device == "imx7d-pico-mbl"
            or device == "imx6ul-pico-mbl"
            or device == "bcm2837-rpi-3-b-32"
            or device == "bcm2837-rpi-3-b-plus-32"
        ):

            return_code, output, error = execute_helper.send_mbl_cli_command(
                [
                    "shell",
                    'sh -l -c "'
                    "{}/bin/pytest "
                    "--verbose "
                    "-s "
                    "--color=no "
                    "{}/mbl-core/ci/lava/tests/test-cellular-and-wifi-dut.py"
                    '"'.format(venv, dut_tutorials_dir),
                ],
                dut_addr,
            )
            # print the output to get any LAVA format strings displayed and
            # processed by LAVA.
            print(output)

            assert True
        else:
            print(
                "Device {} does not support cellular modules.".format(device)
            )
            assert True
