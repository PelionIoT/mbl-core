#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to install test dependencies onto the DUT.

The test is run using pytest and communicates with the DUT using the
mbl-cli which is previously installed as part of the wider LAVA test.

This script also assumes they pytest has been downloaded
into host_download_dir, also as part of the wider LAVA test.
As the LAVA test is using pytest to run this script then reasonable assumption.

"""
import pytest
import re


class Test_Create_Pytest_Environment_On_DUT:
    """Create python test environment on a device under test."""

    def test_download_pyserial_to_host(
        self, execute_helper, host_download_dir
    ):
        """Download pytest into the host."""
        returnCode, put_output, error = execute_helper.execute_command(
            [
                "/tmp/venv/bin/pip3",
                "download",
                "-d",
                host_download_dir,
                "pyserial",
            ]
        )

        assert returnCode == 0

    def test_copy_pytest_to_target(
        self, execute_helper, dut_addr, host_download_dir, dut_download_dir
    ):
        """Copy pytest onto the DUT."""
        copy_success = False

        returnCode, put_output, error = execute_helper.send_mbl_cli_command(
            ["put", "-r", host_download_dir, dut_download_dir], dut_addr
        )

        pattern = re.compile(r"Transfer completed")

        for line in put_output.splitlines():
            match = pattern.match(line)
            if match:
                copy_success = True
                break
        assert copy_success

    def test_create_python_virtual_environment_on_target(
        self, execute_helper, dut_addr, venv
    ):
        """Create the python virtual environment on the DUT."""
        returnCode, output, error = execute_helper.send_mbl_cli_command(
            [
                "shell",
                "python3 -m venv {} --without-pip "
                "--system-site-packages".format(venv),
            ],
            dut_addr,
        )

        assert returnCode == 0

    def test_install_pytest_on_target(
        self, execute_helper, dut_addr, dut_download_dir, venv
    ):
        """Install pytest into the python virtual environment on the DUT."""
        returnCode, output, error = execute_helper.send_mbl_cli_command(
            [
                "shell",
                "source "
                "{}/bin/activate;"
                "pip3 "
                "install "
                "--no-index "
                "--find-links="
                "{} pytest".format(venv, dut_download_dir),
            ],
            dut_addr,
        )

        assert returnCode == 0

    def test_install_pyserial_on_target(
        self, execute_helper, dut_addr, dut_download_dir, venv
    ):
        """Install pytest into the python virtual environment on the DUT."""
        returnCode, output, error = execute_helper.send_mbl_cli_command(
            [
                "shell",
                "source "
                "{}/bin/activate;"
                "pip3 "
                "install "
                "--no-index "
                "--find-links="
                "{} pyserial".format(venv, dut_download_dir),
            ],
            dut_addr,
        )

        assert returnCode == 0
