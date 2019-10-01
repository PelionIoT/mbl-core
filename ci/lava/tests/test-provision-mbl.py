#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Tests for MBL provisioning."""

import os
import time
from pathlib import Path


class TestProvisionMbl:
    """Class to encapsulate the testing of mbl-cli provisioning of a DUT."""

    dut_addr = ""
    certificate = ""

    def test_setup(self, dut_addr, execute_helper):
        """Setup enviroment for tests execution."""
        # Save the dut_addr in the class
        TestProvisionMbl.dut_addr = dut_addr

        # Work out a unique certificate id - the first part of the path is the
        # lava job number.
        current_dir = Path().cwd()
        TestProvisionMbl.certificate = "".join(current_dir.parts[1])

        # Create a working directory for the following tests
        directory = "/tmp/update-resources"
        if not os.path.exists(directory):
            os.mkdir(directory)
        os.chdir(directory)

    def test_manifest_ini(self, execute_helper):
        """Test manifest-tool init command."""
        exit_code, stdout, stderr = execute_helper.execute_command(
            [
                "manifest-tool",
                "init",
                "-q",
                "-d",
                "arm.com",
                "-m",
                "dev-device",
            ]
        )
        if exit_code:
            print(stdout)
            print(stderr)
        assert exit_code == 0

    def test_reject_invalid_key(self, execute_helper):
        """Test invalid API key saving using mbl-cli."""
        exit_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["save-api-key", "invalid_key"], TestProvisionMbl.dut_addr
            ["mbl-cli", "save-api-key", "invalid_key"]
        )
        assert "API key not recognised by Pelion Device Management" in stderr

    def test_pelion_not_configured(self, execute_helper):
        """Test get-pelion-status command when the device is not configured."""
        exit_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["get-pelion-status"], TestProvisionMbl.dut_addr
        )
        assert (
            "Your device is not correctly configured for Pelion Device \
                Management."
            in stderr
        )

    def test_pelion_provisioned(self, execute_helper):
        """Test the actual provisioning of the device."""
        exit_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            [
                "provision-pelion",
                "-c",
                TestProvisionMbl.certificate,
                "anupdatecert",
                "-p",
                "/tmp/update-resources/update_default_resources.c",
            ],
            TestProvisionMbl.dut_addr,
        )
        assert "Provisioning process completed without error." in stdout

    def test_pelion_configured(self, execute_helper):
        """Check if the device has been provisioned correctly."""
        exit_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["get-pelion-status"], TestProvisionMbl.dut_addr
        )
        assert (
            "Device is configured correctly. You can connect to Pelion Cloud!"
            in stdout
        )

    def test_restart_mbl_cloud_client(self, execute_helper):
        """Restart the cloud client and wait 30 seconds to allow connection."""
        exit_code, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl restart mbl-cloud-client"],
            TestProvisionMbl.dut_addr,
        )
        time.sleep(30)
        assert exit_code == 0
