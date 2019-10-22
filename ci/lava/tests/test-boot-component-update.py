#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Pytest for testing boot component update.

This module is testing the update of boot loader components. Those are
wks_bootloader1, wks_bootloaer2 and kernel.
They can be updated via bothe mbl-cli and pelion.
The marker "@pytest.mark.pelion" means that the test will be executed only when
the pelion method is specified
Analogous behaviour with "@pytest.mark.mbl_cli".
If a test case doesn't have any marker, it will be always executed.
"""
import os
import time
import pytest
from pathlib import Path

from helpers import (
    compare_files,
    dd_bootloader_component,
    get_dut_address,
    get_kernel_version,
    strings_grep,
)


class TestBootComponentUpdate:
    """Class to encapsulate the testing of core components on a DUT."""

    update_payload = None

    def test_setup(self, update_payload):
        """Setup test."""
        TestBootComponentUpdate.update_payload = update_payload

    def test_get_current_boot_component_data(
        self, dut_addr, execute_helper, update_component_name
    ):
        """Save data of the component we are about to update.

        For bootloader1 and bootloader2, it saves the whole bootloader
        partition onto a file. This will be then used to compare its content
        after the update.

        For the kernel update it's enough to get the output of "uname -a"
        """
        if update_component_name == "kernel":
            exit_code, output = get_kernel_version(dut_addr, execute_helper)
            assert exit_code == 0
            TestBootComponentUpdate.kernel_version_pre = output
        elif "wks_bootloader" in update_component_name:
            exit_code, component_partition_file = dd_bootloader_component(
                dut_addr,
                execute_helper,
                update_component_name,
                "partition",
                "pre",
            )
            assert exit_code == 0
            TestBootComponentUpdate.component_partition_file_pre = (
                component_partition_file
            )
        else:
            raise AssertionError(
                "Update of component {} not supported".format(
                    update_component_name
                )
            )

    @pytest.mark.pelion
    def test_component_update_pelion(
        self, dut_addr, execute_helper, update_component_name
    ):
        """Test update component using Pelion."""
        pass

    @pytest.mark.mbl_cli
    def test_component_update_mbl_cli(
        self, dut_addr, execute_helper, update_component_name
    ):
        """Test update component using mbl-cli."""
        # Copy the payload onto the DUT
        exit_code, output, error = execute_helper.send_mbl_cli_command(
            ["put", TestBootComponentUpdate.update_payload, "/scratch/"],
            dut_addr,
        )
        assert exit_code == 0

        # Run the update
        payload_name = os.path.basename(TestBootComponentUpdate.update_payload)
        cmd = "mbl-firmware-update-manager --assume-yes /scratch/{}".format(
            payload_name
        )
        exit_code, output, error = execute_helper.send_mbl_cli_command(
            ["shell", 'sh -l -c "{}"'.format(cmd)], dut_addr
        )
        assert "Content of update package installed" in output

    @pytest.mark.mbl_cli
    def test_dut_online_after_reboot(
        self, dut, execute_helper, update_component_name
    ):
        """Wait for the DUT to be back online after the reboot."""
        # Wait some time before discovering again the DUT
        time.sleep(10)
        dut_address = ""
        while not dut_address:
            dut_address = get_dut_address(dut, execute_helper)
            if dut_address:
                print("DUT {} is online again".format(dut_address))
            else:
                print("DUT is still offline. Trying again...")
        assert dut_address

    def test_check_post_update(
        self, dut_addr, execute_helper, update_component_name
    ):
        """Perform post update actions to check if the update succedeed.

        For bootloader1 and bootloader2, the exact portion of the partition is
        dd'ed back and compared with the component file used for the update
        Moreover the printable strings are extracted from both files and
        checked if the build time is different from the pre update file.

        For the kernel update it's enough to get the output of "uname -a" and
        check it it is different from the pre-update one.
        """
        if update_component_name == "kernel":
            exit_code, output = get_kernel_version(dut_addr, execute_helper)
            assert exit_code == 0
            assert output != TestBootComponentUpdate.kernel_version_pre
        elif "wks_bootloader" in update_component_name:
            exit_code, component_file_post = dd_bootloader_component(
                dut_addr, execute_helper, update_component_name, "file", "post"
            )
            assert exit_code == 0

            component_file_pre = (
                Path("/scratch") / update_component_name.upper()
            )
            comparison = compare_files(
                dut_addr,
                execute_helper,
                component_file_pre,
                component_file_post,
            )
            assert comparison == 0

            # String comparison
            # That's the strings command on the partition
            s1 = strings_grep(
                dut_addr,
                execute_helper,
                TestBootComponentUpdate.component_partition_file_pre,
                "Built",
            )
            # That's the strings command on the component
            s2 = strings_grep(
                dut_addr, execute_helper, component_file_post, "Built"
            )
            assert s1 != s2
        else:
            raise AssertionError(
                "Update of component {} not supported".format(
                    update_component_name
                )
            )
