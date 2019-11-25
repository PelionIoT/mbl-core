#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing MBL App Update Manager."""

import importlib
import os
import pytest
import subprocess
from pathlib import Path

import mbl.app_update_manager.cli as aum_cli


UPDATE_PACKAGE_LOCATION = os.path.join(os.sep, "scratch")
MBL_APPS_DIR = os.path.join(os.sep, "home", "app")
USER_APPLICATIONS_ALL_GOOD = [
    "sample-app-1-good",
    "sample-app-2-good",
    "sample-app-3-good",
    "sample-app-4-good",
    "sample-app-5-good",
]
UPDATE_PACKAGE_ALL_APPS_GOOD = "multi-app-all-good.swu"
UPDATE_PACKAGE_ONE_APP_FAILS_INSTALL = "multi-app-one-fail-install.swu"
UPDATE_PACKAGE_ONE_APP_FAILS_RUN = "multi-app-one-fail-run.swu"


class TestMblAppUpdateManager:
    """MBL App Update Manager main class."""

    def test_app_set_can_be_installed(self, uninstall_all_apps):
        """Test that all applications are installed"""
        install_update_package_multiple_times(
            1,
            os.path.join(
                UPDATE_PACKAGE_LOCATION, UPDATE_PACKAGE_ALL_APPS_GOOD
            ),
            USER_APPLICATIONS_ALL_GOOD,
        )

    def test_all_applications_updated(self, uninstall_all_apps):
        """Test that all applications are updated."""
        install_update_package_multiple_times(
            5,
            os.path.join(
                UPDATE_PACKAGE_LOCATION, UPDATE_PACKAGE_ALL_APPS_GOOD
            ),
            USER_APPLICATIONS_ALL_GOOD,
        )

    def test_rollback_if_install_fails(self, uninstall_all_apps):
        """Test that applications are rolledback to previous versions.

        If the cause of failure is inability to install one of the applications
        in the update package.
        """
        # Install the applications
        MAX_INSTALLATION = 2
        install_update_package_multiple_times(
            MAX_INSTALLATION,
            os.path.join(
                UPDATE_PACKAGE_LOCATION, UPDATE_PACKAGE_ALL_APPS_GOOD
            ),
            USER_APPLICATIONS_ALL_GOOD,
        )

        # Attempt to update the applications with an update package that
        # contains an application (sample-app-3-bad-architecture) that cannot
        # be installed.
        # The broken package has an incorrect architecture in the ipk control
        # data file. This causes OPKG to partly install the application and
        # stop half way through installation. `mbl-app-manager` should clean
        # any data saved on the system resulting from a failed installation.
        # The installation should fail and all applications updated before the
        # failure should be reverted.

        # The first installation does not count as an update
        UPDATE_COUNT = MAX_INSTALLATION - 1
        assert (
            install_apps_from_package(
                os.path.join(
                    UPDATE_PACKAGE_LOCATION,
                    UPDATE_PACKAGE_ONE_APP_FAILS_INSTALL,
                )
            )
            == aum_cli.ReturnCode.ERROR.value
        )

        # Check that none of the apps installation paths have changed
        for app in USER_APPLICATIONS_ALL_GOOD:
            app_path = os.path.join(MBL_APPS_DIR, app, str(UPDATE_COUNT))
            assert os.path.isdir(app_path)
            # check that the application parent directory only contains
            # one version
            app_parent_dir = str(Path(app_path).resolve().parent)
            assert len(os.listdir(app_parent_dir)) == 1

        # Check that there is no remnant of the app that failed to install
        bad_app_name = "sample-app-3-bad-architecture"
        assert not os.path.exists(os.path.join(MBL_APPS_DIR, bad_app_name))

    def test_rollback_if_run_fails(self, uninstall_all_apps):
        """Test that applications are rolledback to previous versions.

        If the cause of failure is inability to run one of the applications
        in the update package.
        """
        # Install the applications
        MAX_INSTALLATION = 3
        install_update_package_multiple_times(
            MAX_INSTALLATION,
            os.path.join(
                UPDATE_PACKAGE_LOCATION, UPDATE_PACKAGE_ALL_APPS_GOOD
            ),
            USER_APPLICATIONS_ALL_GOOD,
        )

        # Attempt to update the applications with an update package that
        # contains an application (sample-app-4-bad-oci-runtime) that cannot
        # be run.
        # The broken application has an OCI bundle configuration file with a
        # syntax error thus making it impossible to create a container from the
        # OCI bundle when mbl-app-update-manager tries to run the app via
        # mbl-app-lifecycle-manager.
        # Running the application should fail and all applications updated
        # before the failure should be reverted.

        # The first installation does not count as an update
        UPDATE_COUNT = MAX_INSTALLATION - 1
        assert (
            install_apps_from_package(
                os.path.join(
                    UPDATE_PACKAGE_LOCATION, UPDATE_PACKAGE_ONE_APP_FAILS_RUN
                )
            )
            == aum_cli.ReturnCode.ERROR.value
        )

        # check that none of the apps installation paths have changed
        for app in USER_APPLICATIONS_ALL_GOOD:
            app_path = os.path.join(MBL_APPS_DIR, app, str(UPDATE_COUNT))
            assert os.path.isdir(app_path)
            # Check that the application parent directory only contains
            # one version
            app_parent_dir = str(Path(app_path).resolve().parent)
            assert len(os.listdir(app_parent_dir)) == 1

        # Check that there is no remnant of the app that failed to install
        bad_app_name = "sample-app-4-bad-oci-runtime"
        assert not os.path.exists(os.path.join(MBL_APPS_DIR, bad_app_name))

    def test_app_update_manager_mbl_subpackage(self):
        """
        Test that all modules can be imported as part of the `mbl` namespace
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.app_update_manager.cli") is not None
        assert (
            importlib.__import__("mbl.app_update_manager.manager") is not None
        )
        assert importlib.__import__("mbl.app_update_manager.utils") is not None


def install_apps_from_package(update_pkg):
    """Install user applications."""
    # usage: mbl-app-update-manager [-h] [-v] <update-package>
    print("Update application contained in `{}`".format(update_pkg))
    cmd = ["mbl-firmware-update-manager", "-v", "--keep", update_pkg]
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd, check=False).returncode


@pytest.fixture
def uninstall_all_apps():
    """Delete all user applications installed on the system."""
    for app in USER_APPLICATIONS_ALL_GOOD:
        terminate_app(app)
        remove_app(app, os.path.join(MBL_APPS_DIR, app))
    yield
    for app in USER_APPLICATIONS_ALL_GOOD:
        terminate_app(app)
        remove_app(app, os.path.join(MBL_APPS_DIR, app))


def terminate_app(app_name):
    """Terminate an app."""
    # usage: mbl-app-lifecycle-manager terminate [-h] -n NAME
    #            [-t SIGTERM_TIMEOUT] [-k SIGKILL_TIMEOUT]
    print("Terminating user application `{}`".format(app_name))
    cmd = ["mbl-app-lifecycle-manager", "-v", "terminate", app_name]
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd, check=False)


def remove_app(app_name, app_path):
    """Remove application."""
    # usage: mbl-app-manager remove [-h] app_name app_path
    print("Remove {} from {}".format(app_name, app_path))
    cmd = ["mbl-app-manager", "-v", "remove", app_name, app_path]
    print("Executing command: {}".format(cmd))
    return subprocess.run(cmd, check=False).returncode


def install_update_package_multiple_times(
    installation_number, update_pkg, apps_in_pkg
):
    """Install an update package multiple times."""
    for i in range(0, installation_number):
        assert (
            install_apps_from_package(update_pkg)
            == aum_cli.ReturnCode.SUCCESS.value
        )

        # Check the apps installation paths
        for app in apps_in_pkg:
            app_path = os.path.join(MBL_APPS_DIR, app, str(i))
            assert os.path.isdir(app_path)
            app_parent_dir = str(Path(app_path).resolve().parent)
            # Check that the application parent directory only contains
            # one version
            assert len(os.listdir(app_parent_dir)) == 1
