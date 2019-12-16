#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run systemd tests on the DUT."""

import pytest
import re
import subprocess


class TestBSP:
    """Class to encapsulate the testing of systemd on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestBSP.dut_address = dut_addr

        assert dut_addr

    def test_memtester(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            [
                "shell",
                'sh -l -c "' "memtester  1M 1 | sed 's/:.*ok/: ok/g'" '"',
            ],
            TestBSP.dut_address,
        )
        print(stdout)
        assert err == 0

    def test_optee(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", 'sh -l -c "' "xtest l 0 -t regression" '"'],
            TestBSP.dut_address,
        )
        print(stdout)
        assert err == 0
