#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run systemd tests on the DUT."""

import pytest
import re
import random
import string
import time
from control_cellular import CellularModule as cellular


class TestCellularAndWifiDut:
    """Class to encapsulate the testing of cellular connectivity on a DUT."""

    overall_result = "fail"

    def test_cellular_setup(self, execute_helper):
        """Test the cellular and wifi bevaviour."""
        TestCellularAndWifiDut.overall_result = "pass"

        TestCellularAndWifiDut._debug(self, execute_helper)

    def test_cellular_initialise_cellular_module(self, execute_helper):
        """Enable the cellular interface."""
        assert TestCellularAndWifiDut._control_cellular(self, enable=True)

    def test_cellular_disable_wired_ethernet(self, execute_helper):
        """Bring down the wired interface."""
        assert TestCellularAndWifiDut._disable_interface(
            self, "eth0", execute_helper
        )

    def test_cellular_ping_test_wired_expect_fail(self, execute_helper):
        """Check we cannot ping using the wired interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "eth0", "fail", execute_helper
        )

    def test_cellular_ping_test_cellular_expect_pass_test_1(
        self, execute_helper
    ):
        """Check we can ping using the cellular interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "usb0", "pass", execute_helper
        )

    def test_cellular_disable_cellular(self, execute_helper):
        """Bring down the wired interface."""
        assert TestCellularAndWifiDut._disable_interface(
            self, "usb0", execute_helper
        )

    def test_cellular_wait_for_wlan(self, execute_helper):
        """Wait for 60 seconds to give the wlan time to come up."""
        time.sleep(30)
        test_command = ["connmanctl", "scan", "wifi"]
        err, stdout, stderr = execute_helper.execute_command(test_command)
        time.sleep(30)
        assert err == 0

    def test_cellular_ping_test_wifi_expect_pass(self, execute_helper):
        """Check we can ping using the wifi interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "wlan0", "pass", execute_helper
        )

    def test_cellular_ping_test_cellular_expect_fail(self, execute_helper):
        """Check we cannot ping using the cellular interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "usb0", "fail", execute_helper
        )

    def test_cellular_reenable_cellular_module(self, execute_helper):
        """Re-enable the cellular interface."""
        assert TestCellularAndWifiDut._enable_interface(
            self, "usb0", execute_helper
        )

    def test_cellular_ping_test_wifi_expect_pass(self, execute_helper):
        """Check we cannot ping using the wifi interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "wlan0", "fail", execute_helper
        )

    def test_cellular_ping_test_cellular_expect_pass_test_2(
        self, execute_helper
    ):
        """Check we can ping using the cellular interface."""
        assert TestCellularAndWifiDut._run_ping_tests(
            self, "usb0", "pass", execute_helper
        )

    def test_cellular_cleanup(self, execute_helper):
        """Disable the cellular interface."""
        # Bring down the cellular interface, to be clean.
        TestCellularAndWifiDut._control_cellular(self, enable=False)

        # Bring up the wired interface.
        TestCellularAndWifiDut._enable_interface(self, "eth0", execute_helper)

        assert True

    def test_cellular_overall_result(self, execute_helper):
        """Get overall result."""
        assert TestCellularAndWifiDut.overall_result

    def _run_ping_tests(self, interface, expected, execute_helper):

        TestCellularAndWifiDut._debug(self, execute_helper)
        ipAddressPingStatus = TestCellularAndWifiDut._run_ping_test(
            self, interface, "8.8.8.8", expected, execute_helper
        )

        urlPingStatus = TestCellularAndWifiDut._run_ping_test(
            self, interface, "www.google.com", expected, execute_helper
        )
        return ipAddressPingStatus and urlPingStatus

    def _run_ping_test(self, interface, address, expected, execute_helper):
        """Execute a ping test on an interface."""
        returnValue = False

        test_command = ["ping", "-c", "1", "-I", interface, address]

        err, stdout, stderr = execute_helper.execute_command(test_command)

        if err == 0:
            result = "pass"
            if expected == "pass":
                returnValue = True
            else:
                returnValue = False
                TestCellularAndWifiDut.overall_result = "fail"
        else:
            result = "fail"
            if expected == "pass":
                returnValue = False
                TestCellularAndWifiDut.overall_result = "fail"
            else:
                returnValue = True
        print(
            "Attempted to ping {} using interface {}.".format(
                address, interface
            ),
            "Expected result is {}. Actual result is {}.".format(
                expected, result
            ),
        )
        return returnValue

    def _disable_interface(self, interface, execute_helper):
        test_command = ["ip", "link", "set", interface, "down"]

        err, stdout, stderr = execute_helper.execute_command(test_command)

        # Wait for the interface to be removed and the routing tables
        # etc to be updated
        time.sleep(30)
        return err == 0

    def _enable_interface(self, interface, execute_helper):
        test_command = ["ip", "link", "set", interface, "up"]

        err, stdout, stderr = execute_helper.execute_command(test_command)

        # Wait for the interface to be removed and the routing tables
        # etc to be updated
        time.sleep(30)
        return err == 0

    def _control_cellular(self, enable):
        returnValue = True
        returnValue &= cellular.comms_check()
        time.sleep(1)
        returnValue &= cellular.status()
        time.sleep(1)
        returnValue &= cellular.set_state(enable)
        time.sleep(1)
        returnValue &= cellular.power_cycle()
        return returnValue

    def _debug(self, execute_helper):
        debug_command = ["ifconfig", "-a"]
        err, stdout, stderr = execute_helper.execute_command(debug_command)
        print(stdout)
        debug_command = ["route"]
        err, stdout, stderr = execute_helper.execute_command(debug_command)
        print(stdout)
