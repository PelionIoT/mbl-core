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

# The test expects applications to be copied into the "/home/app" directory on
# the device
dut_app_home = "/home"


class Test_Core_Component_DUT:
    """Class to encapsulate the testing of core components on a DUT."""

    def test_setup(
        self, dut_addr, execute_helper, host_tutorials_dir, dut_tutorials_dir
    ):
        """Copy the test specific parts, generating items as required."""
        execute_helper.send_mbl_cli_command(
            [
                "put",
                "{}/user-sample-app-package_1.0_any.ipk".format(
                    host_tutorials_dir
                ),
                "{}/app-lifecycle-manager-test-package_1.0_any.ipk".format(
                    dut_tutorials_dir
                ),
            ],
            dut_addr,
        )

        execute_helper.send_mbl_cli_command(
            [
                "put",
                "{}/mbl-multi-apps-update-package-all-good.tar".format(
                    host_tutorials_dir
                ),
                dut_tutorials_dir,
            ],
            dut_addr,
        )

        execute_helper.send_mbl_cli_command(
            [
                "put",
                "{}/mbl-multi-apps-update-package-one-fail-run.tar".format(
                    host_tutorials_dir
                ),
                dut_tutorials_dir,
            ],
            dut_addr,
        )

        execute_helper.send_mbl_cli_command(
            [
                "put",
                "{}/mbl-multi-apps-update-package-one-fail-install.tar".format(
                    host_tutorials_dir
                ),
                dut_tutorials_dir,
            ],
            dut_addr,
        )

        # Build the test images
        execute_helper.execute_command(
            [
                "python3",
                "./firmware-management/mbl-app-manager/tests/native/"
                "test_case_generator_mbl-app-manager.py",
                "-o",
                "{}/app".format(host_tutorials_dir),
                "-v",
            ]
        )

        # Copy the test images to the DUT
        execute_helper.send_mbl_cli_command(
            ["put", "-r", "{}/app".format(host_tutorials_dir), dut_app_home],
            dut_addr,
        )

        # Copy the common parts to the DUT
        execute_helper.send_mbl_cli_command(
            ["put", "-r", "./", "{}/mbl-core".format(dut_tutorials_dir)],
            dut_addr,
        )

        # Copy the conftest to the DUT
        execute_helper.send_mbl_cli_command(
            [
                "put",
                "./ci/lava/conftest.py",
                "{}/mbl-core".format(dut_tutorials_dir),
            ],
            dut_addr,
        )

        assert True

    def test_component(
        self, dut_addr, execute_helper, venv, dut_tutorials_dir
    ):
        """Perform the test on the DUT via the mbl-cli."""
        return_code, output, error = execute_helper.send_mbl_cli_command(
            [
                "shell",
                'su -l -c "'
                "{}/bin/pytest "
                "--verbose "
                "--ignore={}/mbl-core/ci --color=no "
                '{}/mbl-core"'.format(
                    venv, dut_tutorials_dir, dut_tutorials_dir
                ),
            ],
            dut_addr,
        )
        # print the output to get any LAVA format strings displayed and
        # processed by LAVA.
        print(output)

        assert True
