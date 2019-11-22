#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run config across reboot tests on the DUT."""

import pytest
import re
import random
import string
import time

from helpers import get_dut_address


class TestConfigAcrossReboot:
    """Class to encapsulate the testing of config settings.

    Modify configuration setting and check they persist after a reboot
    of a DUT.
    """

    originalHostname = None
    newHostname = None
    dut_address = None
    wlan_ip_addr = None

    def test_get_hostname(self, dut, dut_addr, execute_helper):
        """Get the current hostname from the DUT."""
        err, originalHostname = self._get_hostname(
            dut, dut_addr, execute_helper
        )
        TestConfigAcrossReboot.originalHostname = originalHostname
        TestConfigAcrossReboot.dut_address = dut_addr

        assert err == 0
        assert TestConfigAcrossReboot.originalHostname is not None
        assert TestConfigAcrossReboot.dut_address is not None

    def test_change_hostname(self, execute_helper):
        """Change the hostname.

        Append some random digits to the current hostname.
        """
        TestConfigAcrossReboot.newHostname = "{}-{}".format(
            TestConfigAcrossReboot.originalHostname, self._randomStringDigits()
        )
        print(TestConfigAcrossReboot.newHostname)

        """Perform the test on the DUT via the mbl-cli."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            [
                "shell",
                'sh -l -c "echo {} > /config/user/hostname"'.format(
                    TestConfigAcrossReboot.newHostname
                ),
            ],
            TestConfigAcrossReboot.dut_address,
        )
        assert err == 0

    def test_get_wlan_ip(self, execute_helper):
        """Disable the ethernet and get the WLAN IP address"""
        err = self._disable_technology("ethernet", execute_helper)
        assert err == 0

        err, TestConfigAcrossReboot.wlan_ip_address = self._get_wlan_ip(
            execute_helper
        )
        self._ifconfig(execute_helper)
        assert err == 0
        assert TestConfigAcrossReboot.wlan_ip_address is not None

    def test_reboot_dut(self, dut_addr, execute_helper):
        """Cause the DUT to reset."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", 'sh -l -c "/sbin/reboot"'],
            TestConfigAcrossReboot.dut_address,
        )
        # The err code varies and is either 0 of 255 so we cannot depend on it.
        # Making the test pass is reasonable since if the reboot doesn't happen
        # the hostname will not have changed and the next step will fail.
        pass

    def test_dut_online_after_reboot(self, dut, execute_helper):
        """Wait for the DUT to come back online.

        The previous test cause the DUT to reboot.
        """
        dut_address = ""

        # Wait some time before discovering again the DUT
        time.sleep(20)

        while not dut_address:
            dut_address = get_dut_address(dut, execute_helper)
            if dut_address:
                print("DUT {} is online again".format(dut_address))
                break
            print("DUT is still offline. Trying again...")
        assert dut_address is not None

    def test_get_new_hostname(self, dut, dut_addr, execute_helper):
        """Get the hostname from the DUT.

        Check the new hostname matches the new version.
        """
        err, hostname = self._get_hostname(dut, dut_addr, execute_helper)

        assert err == 0
        assert hostname is not None
        assert TestConfigAcrossReboot.newHostname is not None
        assert hostname == TestConfigAcrossReboot.newHostname

    def test_get_wlan_ip_after_reboot(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        self._ifconfig(execute_helper)
        err, wlan_ip_address = self._get_wlan_ip(execute_helper)
        assert err == 0
        assert wlan_ip_address is not None

    def _randomStringDigits(self, stringLength=6):
        """Generate a random string of letters and digits."""
        lettersAndDigits = string.ascii_letters + string.digits
        return "".join(
            random.choice(lettersAndDigits) for i in range(stringLength)
        )

    def _get_hostname(self, dut, dut_addr, execute_helper):
        """Get hostname on the DUT via the mbl-cli."""
        hostname = None

        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", 'sh -l -c "hostname"'], dut_addr
        )

        lines = stdout.splitlines()

        if dut == "auto":
            if len(lines) >= 2:
                hostname = lines[1]
        else:
            for line in lines:
                if dut in line:
                    hostname = line
        if hostname:
            print("_get_hostname: " + hostname)
        else:
            print("_get_hostname: No hostname found")

        return err, hostname

    def _get_wlan_ip(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        wlan_ip_address = None

        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", 'sh -l -c "ifconfig wlan0"'],
            TestConfigAcrossReboot.dut_address,
        )

        if err == 0:
            for line in stdout.splitlines():
                if re.search(r"^.*inet addr:(?:\d{1,3}\.){3}\d{1,3}.*", line):
                    wlan_ip_address = line

        return err, wlan_ip_address

    def _disable_technology(self, technology, execute_helper):
        self._ifconfig(execute_helper)
        test_command = 'sh -l -c "connmanctl disable {}"'.format(technology)

        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", test_command], TestConfigAcrossReboot.dut_address
        )

        # Wait for the interface to be removed and the routing tables
        # etc to be updated
        time.sleep(30)
        self._ifconfig(execute_helper)
        return err

    def _ifconfig(self, execute_helper):
        debug_command = 'sh -l -c "ifconfig"'
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", debug_command], TestConfigAcrossReboot.dut_address
        )
        print(stdout)
        print(stderr)
