#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run avahi discovery test."""

from datetime import datetime
import re
import time


class TestSshProductionImage:
    """Class to encapsulate the avahi discover of a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestSshProductionImage.dut_address = dut_addr
        print(dut_addr)
        assert dut_addr == ""

    def test_mDNS_responder(self, execute_helper):
        """Perform an avahi browse for a single device

        Repeatedly perform avahi-browse calls, until either a
        device is found or 5 minutes have elapsed.
        """
        devices_found = 0
        start_time = datetime.now()
        elapsed_time = 0
        five_minutes = 60 * 5

        while elapsed_time < five_minutes and devices_found == 0:

            devices_found = TestSshProductionImage._run_avahi_browse(self, execute_helper)
            # Pause between commands
            time.sleep(5)
            # calculate long have we been running.
            elapsed_time = (datetime.now() - start_time).seconds

        assert devices_found == 1

    def _run_avahi_browse(self, execute_helper):
        devices_found = 0

        test_command = ["avahi-browse", "-tr", "_ssh._tcp"]
        err, device_list, error = execute_helper.execute_command(
            test_command, 60
        )
        if err == 0:
            # Expecting lines like "=   eno1 IPv4 mbed-linux-os-440"
            # Which contain  with interface, IP version and hostname
            results = re.findall(r"^=\s+(\S*)\s+(\S*)\s+(mbed-linux-os-[0-9]\S*)", device_list, re.M)
            if results:
                for match in results:
                    print("Found: {}/{} - {}".format(match[0],match[1],match[2]))
                devices_found = len(results)
        return devices_found

    #def test_