#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest for testing mbl app lifecycle manager."""

import subprocess
import os
import sys
import importlib
import mbl.applcmgr as alm
import mbl.app_manager as apm


MBL_APP_MANAGER = "mbl-app-manager"
MBL_APP_LIFECYCLE_MANAGER = "mbl-app-lifecycle-manager"
IPK_TEST_FILE = os.path.join(
    os.sep,
    "mnt",
    "cache",
    "app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk",
)
CONTAINER_ID = "test_container_id_1"


class TestAppLifecycleManager:
    """Test App Lifecycle Manager main class."""

    @classmethod
    def setup_class(cls):
        """Setup any state specific to the execution of the given class."""
        print("\nSetup TestAppLifecycleManager")
        cls.app_lifecycle_mgr = alm.AppLifecycleManager()
        cls.app_mgr = apm.AppManager()
        # Install app_lifecycle_mng_test_container
        # All container operation will be done using it
        assert os.path.isfile(IPK_TEST_FILE), "Missing IPK test file."
        # In case container run before the tests - stop it
        command = [
            MBL_APP_LIFECYCLE_MANAGER,
            "-s",
            CONTAINER_ID,
            "-t",
            "3",
            "-v",
        ]
        print("Executing command: {}".format(command))
        subprocess.run(command, check=False)
        print("Installing test container package...")
        command = [MBL_APP_MANAGER, "-f", IPK_TEST_FILE, "-v"]
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        assert result.returncode == 0
        print("Installing test container package - done.\n")
        # Init app_id that will be used in all tests
        cls.app_id = cls.app_mgr.get_package_dest_dir(IPK_TEST_FILE)

    @classmethod
    def teardown_class(cls):
        """Teardown everything previously setup using a call to setup_class."""
        command = [MBL_APP_MANAGER, "-r", cls.app_id, "-v"]
        print("\nTeardown: Deleting test container package...")
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        assert result.returncode == 0
        print("Deleting test container package - Done.")

    def teardown_method(self, method):
        """
        Teardown any state that was previously setup with setup_method call.

        teardown_method is invoked for every test method of a class.
        """
        # Kill container in case test failed
        print("Teardown method start...")
        self._kill_container(CONTAINER_ID, False)
        print("Teardown method end")

    def test_run_container_stop_container(self):
        """
        Test run container and stop container.

        :return: None
        """
        # Run container
        self._run_container(CONTAINER_ID, self.app_id, True)
        state = self.app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.RUNNING
        # Stop container, when done - container should not exist anymore
        self._stop_container(CONTAINER_ID, 10, True)
        state = self.app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.DOES_NOT_EXIST

    def test_run_container_kill_container(self):
        """
        Test run container and kill container (stop is not called).

        :return: None
        """
        # Run container
        self._run_container(CONTAINER_ID, self.app_id, True)
        state = self.app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.RUNNING
        # Kill container, when done - container should not exist anymore
        self._kill_container(CONTAINER_ID, True)
        state = self.app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.DOES_NOT_EXIST

    def test_app_lifecycle_manager_mbl_subpackage(self):
        """
        Test that App Lifecycle Manager is a subpackage of the "mbl" package.

        The App Lifecycle Manager subpackage should be accessible via the
        "mbl" namespace.
        """
        # Assert that the package can be imported as a subpackage to
        assert importlib.__import__("mbl.applcmgr") is not None

    @staticmethod
    def _run_container(CONTAINER_ID, application_id, check_exit_code):
        command = [
            MBL_APP_LIFECYCLE_MANAGER,
            "-r",
            CONTAINER_ID,
            "-a",
            application_id,
            "-v",
        ]
        print("Executing command: {}".format(command))
        result = subprocess.run(command, check=check_exit_code)
        print("_run_container returned {}".format(result.returncode))
        return result.returncode

    @staticmethod
    def _stop_container(CONTAINER_ID, timeout, check_exit_code):
        command = [
            MBL_APP_LIFECYCLE_MANAGER,
            "-s",
            CONTAINER_ID,
            "-t",
            str(timeout),
            "-v",
        ]
        print("Executing command: {}".format(command))
        result = subprocess.run(command, check=check_exit_code)
        print("_stop_container returned {}".format(result.returncode))
        return result.returncode

    @staticmethod
    def _kill_container(CONTAINER_ID, check_exit_code):
        command = [MBL_APP_LIFECYCLE_MANAGER, "-k", CONTAINER_ID, "-v"]
        print("Executing command: {}".format(command))
        result = subprocess.run(command, check=check_exit_code)
        print("_kill_container returned {}".format(result.returncode))
        return result.returncode
