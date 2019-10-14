#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run systemd tests on the DUT."""

import pytest
import re
import time
import pathlib


class TestSystemd:
    """Class to encapsulate the testing of systemd on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestSystemd.dut_address = dut_addr

        assert dut_addr != ""

    def test_systemd_running(self, execute_helper):
        """Perform the test on the DUT via the mbl-cli."""
        # Check status
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl --no-pager is-system-running"],
            TestSystemd.dut_address,
        )
        if err != 0:
            suberr, stdout, stderr = execute_helper.send_mbl_cli_command(
                ["shell", "systemctl --no-pager --failed"],
                TestSystemd.dut_address,
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
            TestSystemd.dut_address,
        )
        assert err == 0

    def test_systemd_watchdog_setup(self, execute_helper):
        """Test the watchdog is enabled and the kernel config is set."""
        # kill systemd.init so the expected messages appear in the dmesg log
        assert (
            execute_helper.send_mbl_cli_command(
                ["shell", "kill 1"], TestSystemd.dut_address
            )[0]
            == 0
        )

        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "dmesg | grep watchdog"], TestSystemd.dut_address
        )
        wdog_timeout_set_re = re.compile(
            r"Set hardware watchdog to (\d*s\.|\dmin\s\ds.)"
        )
        wdog_nowayout_str = "nowayout prevents watchdog being stopped!"

        assert err == 0
        assert not stderr
        assert wdog_nowayout_str in stdout
        assert re.search(wdog_timeout_set_re, stdout).group(0)

    def test_systemd_watchdog_causes_service_sigabrt(self, execute_helper):
        """Test service is terminated when the watchdog timeout is reached."""
        self._setup_wdog_service(execute_helper)

        s_err, s_stdout, s_stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl start wdog-test"], TestSystemd.dut_address
        )
        assert s_err == 0

        # Sleep until the watchdog timeout is exceeded.
        time.sleep(7)

        t_err, t_stdout, t_stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl status wdog-test --no-pager"],
            TestSystemd.dut_address,
        )
        expected_output = (
            "ExecStart=/opt/arm/wdog-test.sh (code=killed, signal=ABRT)"
        )

        assert expected_output in t_stdout

    def _setup_wdog_service(self, execute_helper):
        sp_err, sp_stdout, sp_stderr = execute_helper.send_mbl_cli_command(
            [
                "put",
                "ci/lava/dependencies/wdog-test.service",
                "/etc/systemd/system/wdog-test.service",
            ],
            TestSystemd.dut_address,
        )
        assert sp_err == 0

        tp_err, tp_stdout, tp_stderr = execute_helper.send_mbl_cli_command(
            [
                "put",
                "ci/lava/dependencies/wdog-test.sh",
                "/opt/arm/wdog-test.sh",
            ],
            TestSystemd.dut_address,
        )
        assert tp_err == 0

        cm_err, cm_stdout, cm_stderr = execute_helper.send_mbl_cli_command(
            ["shell", "chmod 755 /opt/arm/wdog-test.sh"],
            TestSystemd.dut_address,
        )
        assert cm_err == 0

        dr_err, dr_stdout, dr_stderr = execute_helper.send_mbl_cli_command(
            ["shell", "systemctl daemon-reload"], TestSystemd.dut_address
        )
        assert dr_err == 0
