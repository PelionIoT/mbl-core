#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run systemd tests on the DUT."""

import pytest
import re
import subprocess


class Test_Systemd:
    """Class to encapsulate the testing of systemd on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        Test_Systemd.dut_address = dut_addr

        assert dut_addr != ""

    def test_systemd_running(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        # Check status
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl --no-pager is-system-running"],
            Test_Systemd.dut_address,
        )
        if err != 0:
            suberr, stdout, stderr = execute_helper.send_mbl_cli_command(
                ["shell", "systemctl --no-pager --failed"],
                Test_Systemd.dut_address,
            )
            print(stdout)
        assert err == 0

    @pytest.mark.parametrize(
        "test_action",
        [
            "boot.mount",
            "config-factory.mount",
            "config-user.mount",
            "home.mount",
            "scratch.mount",
            "tmp.mount",
            "var-log.mount",
            "connman.service",
            "dbus.service",
            "mbl-app-update-manager.service",
            "mbl-cloud-client.service",
            "mbl-dbus-cloud.service",
            "mbl-hostname.service",
            "ofono.service",
            "rngd.service",
            "tee-supplicant.service",
            "wpa_supplicant.service",
        ],
    )
    def test_systemd_service(self, execute_helper, test_action):
        """Perform the test on the DUT via the mbl-cli."""
        # Check status
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl --no-pager status {}".format(test_action)],
            Test_Systemd.dut_address,
        )
        assert err == 0
