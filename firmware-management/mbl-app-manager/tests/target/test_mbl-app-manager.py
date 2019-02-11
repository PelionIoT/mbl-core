#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing MBL App Manager."""

import hashlib
import importlib
import json
import os
import subprocess

import mbl.app_manager.cli as am_cli
import mbl.app_manager.manager as am_m

MBL_APP_MANAGER = "mbl-app-manager"
MBL_APPS_DIR = os.path.join(os.sep, "home", "app")
IPKS_PATH = os.path.join(MBL_APPS_DIR, "test_files")


class TestConfigFile:
    """Represents the test configuration files found in IPKS_PATH."""

    def parse_config(self, config_file):
        """Create an instance of TestConfigFile."""
        with open(config_file, "r") as filelike:
            config = json.load(filelike)

        self.ipk_path = os.path.join(IPKS_PATH, config["ipk_filename"])
        self.pkg_name = config["package_name"]
        self.pkg_desc = config["package_description"]
        self.pkg_arch = config["package_architecture"]
        self.pkg_version = config["package_version"]
        self.install_ret_code = config["return_code_install"]
        self.remove_ret_code = config["return_code_remove"]
        self.files = config["files"]


class TestMblAppManager:
    """MBL App Manager main class."""

    def test_main(self):
        """
        Test main function.

        Parse the configuration files in the IPKS_PATH dir and
        executes the test case it describes.
        """
        ipk_dir_files = os.listdir(IPKS_PATH)
        config_files = [
            os.path.join(IPKS_PATH, ipk_dir_file)
            for ipk_dir_file in ipk_dir_files
            if ipk_dir_file.endswith(".json")
        ]

        for config_file in config_files:
            # Parse config file
            test_config = TestConfigFile()
            test_config.parse_config(config_file)

            print("\n----------------------------------------------------")
            print(
                "Package name: {}\nDescription: {}\nVersion: {}\n"
                "Architecture: {}\n".format(
                    test_config.pkg_name,
                    test_config.pkg_desc,
                    test_config.pkg_version,
                    test_config.pkg_arch,
                )
            )

            app_path = os.path.join(MBL_APPS_DIR, test_config.pkg_name)

            # Remove previously installed packages with the same name.
            # This is needed in case previously test failed and there are
            # still some leftovers
            print("Remove package: {} (cleanup)".format(test_config.pkg_name))
            # Do not check for result because it will fail there was no app
            # to remove... :-|
            remove_app(test_config.pkg_name, app_path)

            # Now install package and check expected returned code
            assert (
                install_app(test_config.ipk_path, app_path)
                == test_config.install_ret_code
            )

            if test_config.install_ret_code == am_cli.ReturnCode.SUCCESS.value:
                # Verify all installed files md5 values
                for filename, expected_md5 in test_config.files.items():
                    file_path = os.path.join(app_path, filename)
                    verify_md5(file_path, expected_md5)

            # Remove installed package and check expected returned code
            print("Remove package: {}".format(test_config.pkg_name))
            assert (
                remove_app(test_config.pkg_name, app_path)
                == test_config.remove_ret_code
            )

    def test_app_manager_mbl_subpackage(self):
        """
        Test that all modules can be imported as part of the `mbl` namespace
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.app_manager.cli") is not None
        assert importlib.__import__("mbl.app_manager.manager") is not None
        assert importlib.__import__("mbl.app_manager.parser") is not None
        assert importlib.__import__("mbl.app_manager.utils") is not None


def verify_md5(file_path, expected_md5):
    """Verify a file md5 values."""
    with open(file_path, "rb") as f:
        buff = f.read()
    md5_hash = hashlib.md5(buff).hexdigest()
    print(
        "Verify {} MD5\nExpected: {}, actual: {}".format(
            file_path, expected_md5, md5_hash
        )
    )
    assert expected_md5 == md5_hash


def install_app(app_pkg, app_path):
    """Install an application."""
    # usage: mbl-app-manager install [-h] -i APP_PACKAGE -p APP_PATH
    print("Install {} at {}".format(app_pkg, app_path))
    command = [MBL_APP_MANAGER, "-v", "install", app_pkg, "-p", app_path]
    print("Executing command: {}".format(command))
    return subprocess.run(command, check=False).returncode


def remove_app(app_name, app_path):
    """Remove an application."""
    # usage: mbl-app-manager remove [-h] -n APP_NAME -p APP_PATH
    print("Remove {} from {}".format(app_name, app_path))
    command = [MBL_APP_MANAGER, "-v", "remove", app_name, "-p", app_path]
    print("Executing command: {}".format(command))
    return subprocess.run(command, check=False).returncode
