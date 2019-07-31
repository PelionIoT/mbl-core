#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to install and run core component tests on the DUT.

This pytest script consists of a test_setup function that installs the
necessary pre-built images and builds and installs images for firmware
testing onto the DUT. It also copies the entire mbl-core clone onto the DUT.

The test_component function runs pytest on the DUT via the mbl-cli on the
mbl-core copy.

"""
import pytest
import re
import subprocess


class Test_Core_Component_DUT:
    """Class to encapsulate the testing of core components on a DUT."""

    def test_setup(self, dut_addr, execute_helper):
        """Copy the test specific parts, generating items as required."""
        execute_helper._send_mbl_cli_command(
            [
                "put",
                "/tmp/user-sample-app-package_1.0_any.ipk",
                "/scratch/app-lifecycle-manager-test-package_1.0_any.ipk",
            ],
            dut_addr,
        )

        execute_helper._send_mbl_cli_command(
            [
                "put",
                "/tmp/mbl-multi-apps-update-package-all-good.tar",
                "/scratch",
            ],
            dut_addr,
        )

        execute_helper._send_mbl_cli_command(
            [
                "put",
                "/tmp/mbl-multi-apps-update-package-one-fail-run.tar",
                "/scratch",
            ],
            dut_addr,
        )

        execute_helper._send_mbl_cli_command(
            [
                "put",
                "/tmp/mbl-multi-apps-update-package-one-fail-install.tar",
                "/scratch",
            ],
            dut_addr,
        )

        # Build the test images
        execute_helper._execute_command(
            [
                "python3",
                "./firmware-management/mbl-app-manager/tests/native/"
                "test_case_generator_mbl-app-manager.py",
                "-o",
                "/tmp/app",
                "-v",
            ]
        )

        # Copy the test images to the DUT
        execute_helper._send_mbl_cli_command(
            ["put", "-r", "/tmp/app", "/home"], dut_addr
        )

        # Copy the common parts to the DUT
        execute_helper._send_mbl_cli_command(
            ["put", "-r", "./", "/scratch/mbl-core"], dut_addr
        )

        # Copy the conftest to the DUT
        execute_helper._send_mbl_cli_command(
            ["put", "./ci/lava/conftest.py", "/scratch/mbl-core"], dut_addr
        )

        assert True

    def test_component(self, dut_addr, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        execute_helper._send_mbl_cli_command(
            [
                "shell",
                "/tmp/venv/bin/pytest "
                "--verbose "
                "--ignore=/scratch/mbl-core/ci --color=no "
                "/scratch/mbl-core",
            ],
            dut_addr,
        )

        assert True
