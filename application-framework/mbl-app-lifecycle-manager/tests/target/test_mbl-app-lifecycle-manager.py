#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing mbl app lifecycle manager."""

import importlib
import os
import subprocess
import sys

import mbl.app_lifecycle_manager.cli as alm_cli
import mbl.app_lifecycle_manager.container as alm_con
import mbl.app_manager.cli as am_cli
import mbl.app_manager.manager as am_m

MBL_APPS_DIR = os.path.join(os.sep, "home", "app")
MBL_APP_MANAGER = "mbl-app-manager"
MBL_APP_LIFECYCLE_MANAGER = "mbl-app-lifecycle-manager"
APP_PKG = os.path.join(
    os.sep,
    "scratch",
    "app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk",
)


class TestAppLifecycleManager:
    """Test App Lifecycle Manager main class."""

    @classmethod
    def setup_class(cls):
        """Set up any state specific to the execution of the given class."""
        print("\nSetup TestAppLifecycleManager")
        cls.app_mgr = am_m.AppManager()
        # Install app_lifecycle_mng_test_container
        # All container operation will be done using it
        assert os.path.isfile(APP_PKG), "Missing IPK test file."
        # In case container run before the tests - stop it
        cls.app_name = cls.app_mgr.get_app_name(APP_PKG)
        terminate_app(cls.app_name, 3, False)

        print("Installing test container package - done.\n")
        # Init app_id that will be used in all tests
        cls.app_path = os.path.join(MBL_APPS_DIR, cls.app_name, "0")

        print("Installing test container package...")
        assert (
            force_install_app(APP_PKG, cls.app_path)
            == am_cli.ReturnCode.SUCCESS.value
        )

    @classmethod
    def teardown_class(cls):
        """Teardown everything previously setup using a call to setup_class."""
        assert (
            remove_app(cls.app_name, cls.app_path)
            == am_cli.ReturnCode.SUCCESS.value
        )
        print("Deleting test container package - Done.")

    def teardown_method(cls, method):
        """
        Teardown any state that was previously setup with setup_method call.

        teardown_method is invoked for every test method of a class.
        """
        # Kill container in case test failed
        print("Teardown method start...")
        assert (
            kill_app(cls.app_name, False) == alm_cli.ReturnCode.SUCCESS.value
        )
        print("Teardown method end")

    # ---------------------------- Test Methods -----------------------------

    def test_run_container_stop_container(cls):
        """
        Test run container and stop container.

        :return: None
        """
        # Test that a container state can be set to 'running'
        assert run_app(cls.app_name, True) == alm_cli.ReturnCode.SUCCESS.value
        state = alm_con.get_state(cls.app_name)
        assert state == alm_con.ContainerState.RUNNING

        # Test that a container state can be set 'stopped'
        assert (
            terminate_app(cls.app_name, 10, True)
            == alm_cli.ReturnCode.SUCCESS.value
        )
        state = alm_con.get_state(cls.app_name)
        assert state == alm_con.ContainerState.DOES_NOT_EXIST

    def test_run_container_kill_container(cls):
        """
        Test run container and kill container (stop is not called).

        :return: None
        """
        # Test that a container state can be set to 'running'
        assert run_app(cls.app_name, True) == alm_cli.ReturnCode.SUCCESS.value
        state = alm_con.get_state(cls.app_name)
        assert state == alm_con.ContainerState.RUNNING

        # Test that runc indicates that the container does not exist
        # after its process has been killed.
        assert kill_app(cls.app_name, True) == alm_cli.ReturnCode.SUCCESS.value
        state = alm_con.get_state(cls.app_name)
        assert state == alm_con.ContainerState.DOES_NOT_EXIST

    def test_app_lifecycle_manager_mbl_subpackage(cls):
        """
        Test that all modules can be imported as part of the `mbl` namespace
        """
        # Assert that the package can be imported as a subpackage to
        assert (
            importlib.__import__("mbl.app_lifecycle_manager.cli") is not None
        )
        assert (
            importlib.__import__("mbl.app_lifecycle_manager.container")
            is not None
        )
        assert (
            importlib.__import__("mbl.app_lifecycle_manager.manager")
            is not None
        )
        assert (
            importlib.__import__("mbl.app_lifecycle_manager.utils") is not None
        )


def run_app(app_name, check_exit_code):
    """Run an app."""
    # usage: mbl-app-lifecycle-manager run [-h] app_name
    command = [MBL_APP_LIFECYCLE_MANAGER, "-v", "run", app_name]
    print("Executing command: {}".format(command))
    process = subprocess.run(command, check=check_exit_code)
    print("run_app returned {}".format(process.returncode))
    return process.returncode


def terminate_app(app_name, timeout, check_exit_code):
    """Terminate an app."""
    # usage: mbl-app-lifecycle-manager terminate [-h] -n NAME
    #            [-t SIGTERM_TIMEOUT] [-k SIGKILL_TIMEOUT]
    command = [
        MBL_APP_LIFECYCLE_MANAGER,
        "-v",
        "terminate",
        app_name,
        "-t",
        str(timeout),
    ]
    print("Executing command: {}".format(command))
    process = subprocess.run(command, check=check_exit_code)
    print("stop_container returned {}".format(process.returncode))
    return process.returncode


def kill_app(app_name, check_exit_code):
    """Kill an app."""
    # usage: mbl-app-lifecycle-manager kill [-h] -n NAME [-t SIGKILL_TIMEOUT]
    command = [MBL_APP_LIFECYCLE_MANAGER, "-v", "kill", app_name]
    print("Executing command: {}".format(command))
    process = subprocess.run(command, check=check_exit_code)
    print("kill_app returned {}".format(process.returncode))
    return process.returncode


def force_install_app(app_pkg, app_path):
    """Remove application if previously installed before installing."""
    # usage: mbl-app-manager force-install [-h] app_package app_path
    print("Force install {} at {}".format(app_pkg, app_path))
    command = [MBL_APP_MANAGER, "-v", "force-install", app_pkg, app_path]
    print("Executing command: {}".format(command))
    return subprocess.run(command, check=False).returncode


def remove_app(app_name, app_path):
    """Remove application."""
    # usage: mbl-app-manager remove [-h] app_name app_path
    print("Remove {} from {}".format(app_name, app_path))
    command = [MBL_APP_MANAGER, "-v", "remove", app_name, app_path]
    print("Executing command: {}".format(command))
    return subprocess.run(command, check=False).returncode
