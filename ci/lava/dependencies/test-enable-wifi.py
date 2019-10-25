#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to install test dependencies onto the DUT.

The test is run using pytest and communicates with the DUT using the
mbl-cli which is previously installed as part of the wider LAVA test.

This script also assumes they pytest has been downloaded
into /tmp, also as part of the wider LAVA test. As the LAVA test is using
pytest to run this script then reasonable assumption.

"""
import os
import pytest
import re
import time

QCADriverUrl = (
    "https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-qca-2.0.3.bin"
)


class Test_Enable_Wifi:
    """Create python test environment on a device under test."""

    def test_enable_wifi(
        self, execute_helper, dut_addr, device, local_conf_file
    ):
        """Enable the Wifi on the device."""
        if (
            device == "imx7s-warp-mbl"
            or device == "bcm2837-rpi-3-b-32"
            or device == "bcm2837-rpi-3-b-plus-32"
        ):
            # Enable WiFi
            ret, put_output, error = execute_helper.send_mbl_cli_command(
                [
                    "put",
                    "-r",
                    "/root/.wifi-access.config",
                    "/config/user/connman/wifi-access.config",
                ],
                dut_addr,
            )

            ret, output, error = execute_helper.send_mbl_cli_command(
                ["shell", "connmanctl enable wifi"], dut_addr
            )
            assert ret == 0

        elif (
            device == "imx7d-pico-mbl"
            or device == "imx8mmevk-mbl"
            or device == "imx6ul-pico-mbl"
        ):

            # Blacklist the p2p interface
            ret, output, error = execute_helper.send_mbl_cli_command(
                [
                    "shell",
                    "sed -i "
                    '"s/#NetworkInterfaceBlacklist='
                    '/NetworkInterfaceBlacklist=p2p/" '
                    "/config/user/connman/main.conf",
                ],
                dut_addr,
            )

            # Force reload of the file
            ret, output, error = execute_helper.send_mbl_cli_command(
                ["shell", 'sh -l -c "systemctl daemon-reload"'], dut_addr
            )

            # Restart connman
            ret, output, error = execute_helper.send_mbl_cli_command(
                ["shell", 'sh -l -c "systemctl restart connman"'], dut_addr
            )

            # Sleep to allow the restart to take effect
            time.sleep(20)

            # Enable WiFi
            ret, put_output, error = execute_helper.send_mbl_cli_command(
                [
                    "put",
                    "-r",
                    "/root/.wifi-access.config",
                    "/config/user/connman/wifi-access.config",
                ],
                dut_addr,
            )

            # Check to see if the firmware is already loaded.
            ret, output, error = execute_helper.send_mbl_cli_command(
                ["shell", "lsmod"], dut_addr
            )

            if "qca9377" not in output:
                # Not loaded so fetch the firmware and install it.

                if os.path.exists(local_conf_file):
                    os.chmod(local_conf_file, 0o777)

                    ret, output, error = execute_helper.send_mbl_cli_command(
                        ["put", local_conf_file, "/tmp"], dut_addr
                    )

                    ret, output, error = execute_helper.send_mbl_cli_command(
                        [
                            "shell",
                            "/tmp/{} --auto-accept".format(
                                os.path.basename(local_conf_file)
                            ),
                        ],
                        dut_addr,
                    )

                    ret, output, error = execute_helper.send_mbl_cli_command(
                        [
                            "shell",
                            "cp -v -r "
                            "firmware-qca-2.0.3/1PJ_QCA9377-3_LEA_2.0/* /",
                        ],
                        dut_addr,
                    )
                    ret, output, error = execute_helper.send_mbl_cli_command(
                        [
                            "shell",
                            "sh -l -c "
                            '"insmod /lib/modules/*/extra/qca9377.ko"',
                        ],
                        dut_addr,
                    )

                    # Wait for the module to be loaded.
                    time.sleep(60)

                    # Enable WiFi
                    ret, output, error = execute_helper.send_mbl_cli_command(
                        ["shell", 'sh -l -c "connmanctl enable wifi"'],
                        dut_addr,
                    )
