#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to install test dependencies onto the DUT.

The test is run using pytest and communicates with the DUT using the
mbl-cli which is previously installed as part of the wider LAVA test.

This script also assumes they pytest has been downloaded
into /tmp, also as part of the wider LAVA test. As the LAVA test is using
pytest to run this script then reasonable assumption.

"""
import pytest
import re


class Test_Create_Pytest_Environment_On_DUT:
    """ Create python test environment on a device under test. """

    def test_copy_component_to_target(self, execute_helper, dut_addr):

        copy_success = False

        returnCode, mbl_cli_put_output, error = execute_helper._send_mbl_cli_command(
            ["put", "-r", "/tmp/pytest", "/tmp/pytest"], dut_addr
        )

        pattern = re.compile(r"Transfer completed")

        for line in mbl_cli_put_output.splitlines():
            match = pattern.match(line)
            if match:
                copy_success = True
                break
        assert copy_success

    def test_create_python_virtual_environment_on_target(
        self, execute_helper, dut_addr
    ):

        returnCode, output, error = execute_helper._send_mbl_cli_command(
            ["shell", "python3 -m venv /tmp/venv --system-site-packages"],
            dut_addr,
        )

        assert returnCode == 0

    def test_install_component_on_target(self, execute_helper, dut_addr):

        returnCode, output, error = execute_helper._send_mbl_cli_command(
            [
                "shell",
                "/tmp/venv/bin/pip3 install --no-index --find-links=/tmp/pytest pytest",
            ],
            dut_addr,
        )

        assert returnCode == 0
