#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Pytest for testing component update.

This module tests the update of image components.

The payload creation script outputs a json file containing information about
the tests we are going to run. We first gather this information from the
testinfo file (and the device if the test requires it). After performing the
update we run some comparisons of the testinfo data gathered before and after
the update.

The updates can be performed using both the mbl-cli and the pelion device
management API, the latter is accessed using the manifest-tool.
The marker "@pytest.mark.pelion" means that the test will be executed only when
the pelion method is specified on the command line.
Analogous behaviour with "@pytest.mark.mbl_cli".
If a test case doesn't have any marker, it will be always executed.
"""

import json
import os
import time

import pytest

from helpers import (
    read_partition_to_file,
    get_dut_address,
    get_file_contents_md5,
    get_file_sha256sum,
    get_app_info,
    get_mounted_bank_device_name,
    get_file_mtime,
    strings_grep,
    get_pelion_device_id,
)


def _get_testinfo_from_json(filename):
    if filename is None:
        raise FileNotFoundError("Failed to find {}".format(filename))

    with open(filename) as fobj:
        return json.loads(fobj.read())


file_cmp_test_info = []
mounted_bank_test_info = []
file_timestamp_test_info = []
part_sha_256_test_info = []
file_sha256_test_info = []
app_bank_test_info = []


class TestComponentUpdate:
    """Class to encapsulate the testing of core components on a DUT."""

    update_payload = None
    dut_address = None

    def test_setup(self, update_payload, dut_addr):
        """Setup test."""
        TestComponentUpdate.update_payload = update_payload
        TestComponentUpdate.dut_address = dut_addr
        assert TestComponentUpdate.dut_address
        assert TestComponentUpdate.update_payload

    def test_get_pre_update_testinfo(
        self, execute_helper, update_payload_testinfo, update_component_name
    ):
        """Gather test info from the image before updating it."""
        expected_test_info = _get_testinfo_from_json(update_payload_testinfo)

        for image_data in expected_test_info["images"]:
            for item in image_data["tests"]:
                if item["test_type"] == "file_compare":
                    file_cmp_test_info.append(
                        (
                            image_data["image_name"],
                            item["args"],
                            get_file_contents_md5(
                                **item["args"],
                                dut_addr=TestComponentUpdate.dut_address,
                                execute_helper=execute_helper
                            ),
                        )
                    )
                elif item["test_type"] == "mounted_bank_compare":
                    mounted_bank_test_info.append(
                        (
                            image_data["image_name"],
                            item["args"],
                            get_mounted_bank_device_name(
                                **item["args"],
                                dut_addr=TestComponentUpdate.dut_address,
                                execute_helper=execute_helper
                            ),
                        )
                    )
                elif item["test_type"] == "file_timestamp_compare":
                    file_timestamp_test_info.append(
                        (
                            image_data["image_name"],
                            item["args"],
                            get_file_mtime(
                                **item["args"],
                                dut_addr=TestComponentUpdate.dut_address,
                                execute_helper=execute_helper
                            ),
                        )
                    )
                elif item["test_type"] == "partition_sha256":
                    pre_update_img = read_partition_to_file(
                        TestComponentUpdate.dut_address,
                        execute_helper,
                        item["args"]["part_name"],
                        "pre",
                        item["args"]["size_B"],
                    )
                    pre_timestamp = strings_grep(
                        TestComponentUpdate.dut_address,
                        execute_helper,
                        pre_update_img,
                        "Built",
                    )
                    part_sha_256_test_info.append(
                        (image_data["image_name"], item["args"], pre_timestamp)
                    )
                elif item["test_type"] == "file_sha256":
                    file_sha256_test_info.append(
                        (image_data["image_name"], item["args"])
                    )
                elif item["test_type"] == "app_bank_compare":
                    app_bank_test_info.append(
                        (image_data["image_name"], item["args"])
                    )
        assert not all(
            not x
            for x in [
                file_cmp_test_info,
                file_sha256_test_info,
                part_sha_256_test_info,
                file_timestamp_test_info,
                mounted_bank_test_info,
                app_bank_test_info,
            ]
        )

    @pytest.mark.pelion
    def test_component_update_pelion(
        self, execute_helper, update_component_name
    ):
        """Test update component using Pelion."""
        # Move to the working directory for the manifest tool.
        # This directory was where the provisioning test ran
        # the `manifest-tool init` command.
        directory = "/tmp/update-resources"
        os.chdir(directory)

        dev_id = get_pelion_device_id(
            TestComponentUpdate.dut_address, execute_helper
        )
        exit_code, output, err = execute_helper.execute_command(
            [
                "manifest-tool",
                "update",
                "device",
                "--device-id",
                dev_id,
                "--payload",
                TestComponentUpdate.update_payload,
            ]
        )

        assert exit_code == 0

    @pytest.mark.mbl_cli
    def test_component_update_mbl_cli(
        self, execute_helper, update_component_name
    ):
        """Test update component using mbl-cli."""
        # Copy the payload onto the DUT
        execute_helper.send_mbl_cli_command(
            ["put", TestComponentUpdate.update_payload, "/scratch/"],
            TestComponentUpdate.dut_address,
            raise_on_error=True,
        )

        # Run the update
        payload_name = os.path.basename(TestComponentUpdate.update_payload)
        cmd = "mbl-firmware-update-manager --assume-no /scratch/{}".format(
            payload_name
        )
        try:
            _, output, err = execute_helper.send_mbl_cli_command(
                ["shell", 'sh -l -c "{}"'.format(cmd)],
                TestComponentUpdate.dut_address,
                raise_on_error=True,
            )
        except AssertionError:
            _, output, err = execute_helper.send_mbl_cli_command(
                ["shell", "cat /var/log/arm_update_activate.log"],
                TestComponentUpdate.dut_address,
                raise_on_error=True,
            )
            print(output)
            raise
        assert "Content of update package installed" in output

        # Don't reboot the device in case of app updates
        if update_component_name not in ["sample-app", "multi-app-all-good"]:
            execute_helper.send_mbl_cli_command(
                ["shell", 'sh -l -c "reboot || true"'],
                TestComponentUpdate.dut_address,
            )

    @pytest.mark.mbl_cli
    def test_dut_online_after_reboot(
        self, dut, execute_helper, update_component_name
    ):
        """Wait for the DUT to be back online after the reboot."""
        # Wait some time before attempting to discover the DUT
        if update_component_name not in ["sample-app", "multi-app-all-good"]:
            time.sleep(30)
            dut_address = ""
            num_retries = 0
            while not dut_address and num_retries < 30:
                dut_address = get_dut_address(dut, execute_helper)
                if dut_address:
                    print("DUT {} is online again".format(dut_address))
                else:
                    print("DUT is still offline. Trying again...")
                num_retries += 1
            assert dut_address
        else:
            pytest.skip("Don't reboot for app updates")

    def test_image_post_update_checks(
        self, execute_helper, update_component_name
    ):
        """Run some comparisons against the testinfo data."""
        if part_sha_256_test_info:
            self._compare_partition_sha256(execute_helper)

        if file_sha256_test_info:
            self._compare_file_sha256(execute_helper)

        if file_cmp_test_info:
            self._compare_files(execute_helper)

        if file_timestamp_test_info:
            self._compare_file_timestamps(execute_helper)

        if mounted_bank_test_info:
            self._compare_mounted_bank(execute_helper)

        if app_bank_test_info:
            self._compare_app_info(execute_helper)

    def _compare_partition_sha256(self, execute_helper):
        for item in part_sha_256_test_info:
            img_name, data, before_result = item
            print("Testing partition sha256 for image {}".format(img_name))
            post_extracted_img = read_partition_to_file(
                TestComponentUpdate.dut_address,
                execute_helper,
                data["part_name"],
                "post",
                data["size_B"],
            )
            assert data["sha256"] == get_file_sha256sum(
                post_extracted_img,
                TestComponentUpdate.dut_address,
                execute_helper,
            )
            assert (
                strings_grep(
                    TestComponentUpdate.dut_address,
                    execute_helper,
                    post_extracted_img,
                    "Built",
                )
                != before_result
            )

    def _compare_file_sha256(self, execute_helper):
        for item in file_sha256_test_info:
            img_name, data = item
            print("Testing file sha256 for image {}".format(img_name))
            assert data["sha256"] == get_file_sha256sum(
                data["path"], TestComponentUpdate.dut_address, execute_helper
            )

    def _compare_files(self, execute_helper):
        for item in file_cmp_test_info:
            img_name, data, before_result = item
            print("Testing file comparison for image {}".format(img_name))
            assert before_result != get_file_contents_md5(
                data["path"], TestComponentUpdate.dut_address, execute_helper
            )

    def _compare_mounted_bank(self, execute_helper):
        for item in mounted_bank_test_info:
            img_name, data, before_result = item
            print("Testing mounted bank for image {}".format(img_name))
            assert before_result != get_mounted_bank_device_name(
                data["mount_point"],
                TestComponentUpdate.dut_address,
                execute_helper,
            )

    def _compare_file_timestamps(self, execute_helper):
        for item in file_timestamp_test_info:
            img_name, data, before_result = item
            print("Testing file timestamps for image {}".format(img_name))
            assert before_result != get_file_mtime(
                data["path"], TestComponentUpdate.dut_address, execute_helper
            )

    def _compare_app_info(self, execute_helper):
        for item in app_bank_test_info:
            img_name, data = item
            app_name = data["app_name"]
            app_info = get_app_info(
                app_name,
                TestComponentUpdate.dut_address,
                execute_helper,
                app_output=False,
            )
            print(
                "Checking the app {} is running and installed".format(app_name)
            )
            assert '"status": "running"' in app_info
            assert '"bundle": "/home/app/{}/0'.format(app_name) in app_info
        # Wait 30 seconds for the app to stop.
        time.sleep(30)
        for item in app_bank_test_info:
            img_name, data = item
            app_name = data["app_name"]
            app_info = get_app_info(
                app_name,
                TestComponentUpdate.dut_address,
                execute_helper,
                app_output=True,
            )
            print(
                "Checking the app {} is stopped and its output".format(
                    app_name
                )
            )
            assert '"status": "stopped"' in app_info
            assert app_info.count("Hello, world") == 10
