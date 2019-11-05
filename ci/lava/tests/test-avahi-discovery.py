#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run avahi discovery test."""

from datetime import datetime
import re
import time


class TestRunner:
    """Class to encapsulate the avahi discover of a DUT."""

    def test_avahi_browse(self, execute_helper):
        """Perform the avahi browse test.

        Repeatedly perform avahi-browse calls, one per second, until either a
        device is found or 5 minutes have elapsed.
        """
        device_found = False
        start_time = datetime.now()
        elapsed_time = 0
        five_minutes = 60 * 5

        while elapsed_time < five_minutes and device_found is False:

            device_found = TestRunner._run_avahi_browse(self, execute_helper)

            # Pause between commands
            time.sleep(1)

            # calculate long have we been running.
            elapsed_time = (datetime.now() - start_time).seconds

        assert device_found

    def _run_avahi_browse(self, execute_helper):
        device_found = False

        test_command = ["avahi-browse", "-tr", "_ssh._tcp"]
        err, device_list, error = execute_helper.execute_command(
            test_command, 60
        )

        # Look fo a line starting with "=" and containing "mbed-linux-os"
        if err == 0:
            if re.search(r"^=.*mbed-linux-os", device_list, re.M):
                device_found = True
        return device_found
