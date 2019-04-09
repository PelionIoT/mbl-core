#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing MBL Firmware Update Manager."""

import importlib
import os
import pytest
import subprocess

import mbl.firmware_update_manager.cli as fum_cli


UPDATE_PACKAGE_LOCATION = os.path.join(os.sep, "scratch")
MBL_APPS_DIR = os.path.join(os.sep, "home", "app")
USER_APPS_UPDATE_PACKAGE = "mbl-multi-apps-update-package-all-good.tar"
ROOTFS_UPDATE_PACKAGE = "rootfs-update-package.tar"
USER_APPLICATIONS = [
    "sample-app-1-good",
    "sample-app-2-good",
    "sample-app-3-good",
    "sample-app-4-good",
    "sample-app-5-good",
]
UPDATE_FLAG_FILE = os.path.join(os.sep, "tmp", "do_not_reboot")


class TestMblFirmwareUpdateManager:
    """MBL Firmware Update Manager main class."""

    def test_app_update_pkg_can_be_installed(self, uninstall_all_apps):
        """Test that application update package can be installed."""
        assert (
            install_fmw_from_package(
                os.path.join(UPDATE_PACKAGE_LOCATION, USER_APPS_UPDATE_PACKAGE)
            )
            == fum_cli.ReturnCode.SUCCESS.value
        )

        # Check that apps are on the filesystem
        for app in USER_APPLICATIONS:
            app_parent_dir = os.path.join(MBL_APPS_DIR, app)
            assert os.path.isdir(app_parent_dir)

    @pytest.mark.skip(
        reason=(
            "need to get appropriate rootfs for the platform"
            " on the device before running this"
        )
    )
    def test_rootfs_update_pkg_can_be_installed(self, setup_update_flag_file):
        """Test that rootfs update package can be installed."""
        assert (
            install_fmw_from_package(
                os.path.join(UPDATE_PACKAGE_LOCATION, ROOTFS_UPDATE_PACKAGE)
            )
            == fum_cli.ReturnCode.SUCCESS.value
        )

        # Check that the update flag file has been deleted as it was a rootfs
        # update
        assert os.path.isfile(UPDATE_FLAG_FILE) is False

    def test_firmware_update_manager_mbl_subpackage(self):
        """
        Test that Firmware Update Manager is a subpackage of the "mbl" package.

        The Firmware Update Manager subpackage should be accessible via the
        "mbl" namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert (
            importlib.__import__("mbl.firmware_update_manager.cli") is not None
        )
        assert (
            importlib.__import__("mbl.firmware_update_manager.manager")
            is not None
        )
        assert (
            importlib.__import__("mbl.firmware_update_manager.utils")
            is not None
        )


def install_fmw_from_package(update_pkg, reboot=False):
    """Install firmware."""
    # usage: mbl-firmware-update-manager [-h] [--assume-no | --assume-yes]
    #                                [--keep]
    #                                [-v]
    #                                <update-package>
    print("Update firmware contained in `{}`".format(update_pkg))
    cmd = (
        "mbl-firmware-update-manager",
        "-v",
        update_pkg,
        "--keep",
        "--assume-no",
    )
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd).returncode


@pytest.fixture
def uninstall_all_apps():
    """Delete all user applications installed on the system."""
    for app in USER_APPLICATIONS:
        terminate_app(app)
        remove_app(app, os.path.join(MBL_APPS_DIR, app))
    yield
    for app in USER_APPLICATIONS:
        terminate_app(app)
        remove_app(app, os.path.join(MBL_APPS_DIR, app))


@pytest.fixture
def setup_update_flag_file():
    """Ensure the flag file exists before the test and clear it after."""
    open(UPDATE_FLAG_FILE, "a").close()
    yield
    if os.path.isfile(UPDATE_FLAG_FILE):
        os.remove(UPDATE_FLAG_FILE)


def terminate_app(app_name):
    """Terminate an app."""
    # usage: mbl-app-lifecycle-manager terminate [-h] -n NAME
    #            [-t SIGTERM_TIMEOUT] [-k SIGKILL_TIMEOUT]
    print("Terminating user application `{}`".format(app_name))
    cmd = ("mbl-app-lifecycle-manager", "-v", "terminate", app_name)
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd)


def remove_app(app_name, app_path):
    """Remove application."""
    # usage: mbl-app-manager remove [-h] app_name app_path
    print("Remove {} from {}".format(app_name, app_path))
    cmd = ("mbl-app-manager", "-v", "remove", app_name, app_path)
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd).returncode
