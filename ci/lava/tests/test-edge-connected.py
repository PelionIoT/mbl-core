#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run endge connected test on the DUT."""

import pytest
import re


class Test_Edge_Connected:
    """Class to encapsulate the testing of systemd on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        Test_Edge_Connected.dut_address = dut_addr

        assert dut_addr != ""

    def test_edge_connected(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        # Check status
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["get", "/var/log/edge-core.log", "/tmp/edge-core.log"],
            Test_Edge_Connected.dut_address,
        )

        if err == 0:
            lineFound = False

            # Parse the logfile, looking for "Endpoint id"
            with open("/tmp/edge-core.log", "rt") as log:
                for line in log:
                    if line.find("Endpoint id") != -1:
                        print(line)
                        lineFound = True

            if lineFound is False:
                err = 1

        else:
            print("Failed to get log file from device")

        assert err == 0
