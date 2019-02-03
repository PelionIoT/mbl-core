#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing MBL App Manager."""

import subprocess
import json
import os
import hashlib
import importlib

MBL_APP_MANAGER = "/usr/bin/mbl-app-manager"
IPK_TEST_FILES_DIR = "/home/app/test_files/"
APPS_INSTALL_ROOT_DIR = "/home/app"


class TestMblAppManager:
    """MBL App Manager main class."""

    def test_main(self):
        """
        Test main function.

        This test goes over all JSON files in the "test_files" directory, parse
        the configuration and execute the test case it describes.
        :return: None
        """
        subdirectories = os.listdir(IPK_TEST_FILES_DIR)
        for sub_directory in subdirectories:
            filename = os.path.join(IPK_TEST_FILES_DIR, sub_directory)
            if os.path.splitext(filename)[1] != ".json":
                continue

            # Parse JSON file
            with open(filename, "r") as infile:
                text = infile.read()
                test_case_config = json.loads(text)
                ipk_filename = os.path.join(
                    IPK_TEST_FILES_DIR, test_case_config["ipk_filename"]
                )
                package_name = test_case_config["package_name"]
                package_description = test_case_config["package_description"]
                package_architecture = test_case_config["package_architecture"]
                package_version = test_case_config["package_version"]
                return_code_install = test_case_config["return_code_install"]
                return_code_remove = test_case_config["return_code_remove"]
                files = test_case_config["files"]

                print("\n----------------------------------------------------")
                print(
                    "Package name: {}\nDescription: {}\nVersion: {}\n"
                    "Architecture: {}\n".format(
                        package_name,
                        package_description,
                        package_version,
                        package_architecture,
                    )
                )
                # Remove previously installed packages with the same name.
                # This is needed in case previously test failed and there are
                # still some leftovers
                self._remove_package(package_name, 0, True)
                # Now install and verify md5 values for every file in the
                # package
                self._install_package(
                    ipk_filename, package_name, return_code_install, files
                )
                # Remove installed package and check return code
                self._remove_package(package_name, return_code_remove)

    def _run_app_manager(
        self, command, expected_return_code, ignore_return_code=False
    ):
        p = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=-1,
        )
        output, error = p.communicate()

        if not ignore_return_code:
            print(
                "App manager returned: {}, expected: {}.\n"
                "Output: {}\n{}".format(
                    p.returncode,
                    expected_return_code,
                    output.decode("utf-8"),
                    error.decode("utf-8"),
                )
            )
            assert p.returncode == expected_return_code

    def _remove_package(
        self, package_name, expected_return_code, ignore_return_code=False
    ):
        """
        Remove installed package.

        :param package_name: Package name
        :param ignore_return_code: If set to True - ignore remove return code.
               Using this function before every run is needed in order to clean
               (possible) previous failing tests.
        :return: None
        """
        if ignore_return_code:
            print("Remove package: {} (cleanup)".format(package_name))
        else:
            print("Remove package: {}".format(package_name))
        command = ["python3", MBL_APP_MANAGER, "-r", package_name]
        self._run_app_manager(
            command, expected_return_code, ignore_return_code
        )

    def _install_package(
        self, ipk_filename, package_name, expected_return_code, files
    ):
        # Install ipk
        print("Installing package: {}".format(package_name))
        command = ["python3", MBL_APP_MANAGER, "-i", ipk_filename]
        self._run_app_manager(command, expected_return_code)

        # In case installation is expected to fail - no need to check md5
        if expected_return_code is not 0:
            return

        # Verify installed files md5 values
        for key, val in files.items():
            file_full_path = os.path.join(
                APPS_INSTALL_ROOT_DIR, package_name, key
            )
            expected_md5 = val
            with open(file_full_path, "rb") as infile:
                buff = infile.read()
                md5_hash = hashlib.md5(buff).hexdigest()
                print(
                    "Verify MD5 of: {}\n   Expected: {}, actual: {}".format(
                        file_full_path, expected_md5, md5_hash
                    )
                )
                assert expected_md5 == md5_hash

    def test_app_manager_mbl_subpackage(self):
        """
        Test that App Manager is a subpackage of the "mbl" package.

        The App Manager subpackage should be accessible via the "mbl"
        namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.app_manager.manager") is not None
