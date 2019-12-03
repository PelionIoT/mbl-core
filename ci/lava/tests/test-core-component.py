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
import os
import pytest
import re

from helpers import download_from_url


# The test expects applications to be copied into the "/home/app" directory on
# the device
dut_app_home = "/home"


class TestCoreComponentDUT:
    """Class to encapsulate the testing of core components on a DUT."""

    local_conf_file = None

    def test_setup(
        self,
        dut_addr,
        execute_helper,
        host_artifacts_dir,
        dut_artifacts_dir,
        local_conf_file,
        payloads_url,
    ):
        """Copy the test specific parts, generating items as required."""
        print("Download and copy app payloads to the DUT")
        self._download_and_copy_app_payloads(
            payloads_url, execute_helper, dut_artifacts_dir, dut_addr
        )

        print("Copy the local.conf file, creating the directory first.")
        return_code, output, error = execute_helper.send_mbl_cli_command(
            ["shell", "mkdir -p {}".format(os.path.dirname(local_conf_file))],
            dut_addr,
        )
        execute_helper.send_mbl_cli_command(
            ["put", local_conf_file, local_conf_file], dut_addr
        )
        TestCoreComponentDUT.local_conf_file = local_conf_file

        print("Build the test images")
        execute_helper.execute_command(
            [
                "python3",
                "./firmware-management/mbl-app-manager/tests/native/"
                "test_case_generator_mbl-app-manager.py",
                "-o",
                "{}/app".format(host_artifacts_dir),
                "-v",
            ]
        )

        print("Copy the test images to the DUT")
        execute_helper.send_mbl_cli_command(
            ["put", "-r", "{}/app".format(host_artifacts_dir), dut_app_home],
            dut_addr,
        )

        print("Copy the common parts to the DUT")
        execute_helper.send_mbl_cli_command(
            ["put", "-r", "./", "{}/mbl-core".format(dut_artifacts_dir)],
            dut_addr,
        )

        print("Copy the conftest to the DUT")
        execute_helper.send_mbl_cli_command(
            [
                "put",
                "./ci/lava/conftest.py",
                "{}/mbl-core".format(dut_artifacts_dir),
            ],
            dut_addr,
        )

        print("Copy the helpers.py to the DUT")
        execute_helper.send_mbl_cli_command(
            [
                "put",
                "./ci/lava/helpers.py",
                "{}/mbl-core".format(dut_artifacts_dir),
            ],
            dut_addr,
        )
        assert True

    def test_component(
        self, dut_addr, execute_helper, venv, dut_artifacts_dir
    ):
        """Perform the test on the DUT via the mbl-cli."""
        return_code, output, error = execute_helper.send_mbl_cli_command(
            [
                "shell",
                'sh -l -c "'
                "{}/bin/pytest "
                "--verbose "
                "--ignore={}/mbl-core/ci --color=no "
                "{}/mbl-core "
                '--local-conf-file {}"'.format(
                    venv,
                    dut_artifacts_dir,
                    dut_artifacts_dir,
                    TestCoreComponentDUT.local_conf_file,
                ),
            ],
            dut_addr,
        )
        # print the output to get any LAVA format strings displayed and
        # processed by LAVA.
        print(output)

        assert return_code == 0

    def _download_and_copy_app_payloads(
        self, payloads_url, execute_helper, dut_artifacts_dir, dut_addr
    ):
        payloads = [
            "sample-app",
            "multi-app-all-good",
            "multi-app-one-fail-install",
            "multi-app-one-fail-run",
        ]
        for payload in payloads:
            local_file = download_from_url(
                "{0}/{1}.swu/{1}.swu".format(payloads_url, payload)
            )
            execute_helper.send_mbl_cli_command(
                [
                    "put",
                    local_file,
                    "{}/{}".format(
                        dut_artifacts_dir, os.path.basename(local_file)
                    ),
                ],
                dut_addr,
            )
