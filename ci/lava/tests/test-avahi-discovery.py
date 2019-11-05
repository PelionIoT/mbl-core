#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run avahi discovery test."""

from datetime import datetime
import time


class TestRunner:
    """Class to encapsulate the avahi discover of a DUT."""

    def _test_avahi_browse(self, execute_helper):
        returnValue = False

        test_command = ["avahi-browse", "-tr", "_ssh._tcp"]
        err, device_list, error = execute_helper.execute_command(
            test_command, 60
        )

        if err == 0:
            lines = device_list.split("\n")
            for line in lines:
                if line.startswith("=") and "mbed-linux-os" in line:
                    returnValue = True
                    break
        return returnValue

    def test_avahi_browse(self, execute_helper):
        """Perform the avahi browse test.

        Repeatedly perform avahi-browse calls, one per second, until either a
        device is found or 5 minutes have elapsed.
        """
        returnValue = False
        start_time = datetime.now()

        while (
            not returnValue and (datetime.now() - start_time).seconds < 5 * 60
        ):
            returnValue = TestRunner._test_avahi_browse(self, execute_helper)
            time.sleep(1)
        assert returnValue
