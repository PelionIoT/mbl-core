#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest for testing mbl app lifecycle manager."""

import subprocess
import os
import sys
import mbl.AppLifecycleManager as alm

MBL_APP_MANAGER = "mbl-app-manager"
MBL_APP_LIFECYCLE_MANAGER = "mbl-app-lifecycle-manager"
IPK_TEST_FILE = "/app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk"
TEST_APP_ID = "app-lifecycle-manager-test-package"
CONTAINER_ID = "test_container_id_1"


class TestAppLifecycleManager:
    """Test App Lifecycle Manager main class."""

    @classmethod
    def setup_class(cls):
        """Setup any state specific to the execution of the given class."""
        # Install app_lifecycle_mng_test_container
        # All container operation will be done using it
        assert os.path.isfile(IPK_TEST_FILE), "Missing IPK test file."
        command = [MBL_APP_MANAGER, "-f", IPK_TEST_FILE, "-v"]
        print("\nSetup: Installing test container package...")
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        assert result.returncode == 0
        print("Installing test container package - done.\n")

    @classmethod
    def teardown_class(cls):
        """Teardown everything previously setup using a call to setup_class."""
        command = [MBL_APP_MANAGER, "-r", TEST_APP_ID, "-v"]
        print("\nTeardown: Deleting test container package...")
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        assert result.returncode == 0
        print("Deleting test container package - Done.")

    def setup_method(self, method):
        """
        Setup any state tied to the execution of the given method in a class.

        setup_method is invoked for every test method of a class.
        """
        print(
            "\n======== Test module:{}, Test name: {} ========".format(
                self.__class__.__name__, method.__name__
            )
        )

    def teardown_method(self, method):
        """
        Teardown any state that was previously setup with setup_method call.

        teardown_method is invoked for every test method of a class.
        """
        # Kill container in case test failed
        print("Teardown method start...")
        self._kill_container(CONTAINER_ID)
        print("Teardown method end")

    def test_run_container_stop_container(self):
        """
        Test run container and stop container.

        :return: None
        """
        # Run container
        app_lifecycle_mgr = alm.AppLifecycleManager()
        result = self._run_container(CONTAINER_ID, TEST_APP_ID)
        assert result == alm.Error.SUCCESS.value
        state = app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.RUNNING
        # Stop container, when done - container should not exist anymore
        result = self._stop_container(CONTAINER_ID, timeout="10")
        assert result == alm.Error.SUCCESS.value
        state = app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.DOES_NOT_EXIST

    def test_run_container_kill_container(self):
        """
        Test run container and kill container (stop is not called).

        :return: None
        """
        # Run container
        app_lifecycle_mgr = alm.AppLifecycleManager()
        result = self._run_container(CONTAINER_ID, TEST_APP_ID)
        assert result == alm.Error.SUCCESS.value
        state = app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.RUNNING
        # Kill container, when done - container should not exist anymore
        result = self._kill_container(CONTAINER_ID)
        assert result == alm.Error.SUCCESS.value
        state = app_lifecycle_mgr.get_container_state(CONTAINER_ID)
        assert state == alm.ContainerState.DOES_NOT_EXIST

    @staticmethod
    def _run_container(CONTAINER_ID, application_id):
        command = [
            MBL_APP_LIFECYCLE_MANAGER,
            "-r",
            CONTAINER_ID,
            "-a",
            application_id,
            "-v",
        ]
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        print("_run_container returned {}".format(result.returncode))
        return result.returncode

    @staticmethod
    def _stop_container(CONTAINER_ID, timeout):
        command = [
            MBL_APP_LIFECYCLE_MANAGER,
            "-s",
            CONTAINER_ID,
            "-t",
            timeout,
            "-v",
        ]
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        print("_stop_container returned {}".format(result.returncode))
        return result.returncode

    @staticmethod
    def _kill_container(CONTAINER_ID):
        command = [MBL_APP_LIFECYCLE_MANAGER, "-k", CONTAINER_ID, "-v"]
        print("Executing command: {}".format(command))
        result = subprocess.run(command)
        print("_kill_container returned {}".format(result.returncode))
        return result.returncode
