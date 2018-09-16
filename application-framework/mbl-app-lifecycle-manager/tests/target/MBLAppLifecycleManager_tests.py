#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Pytest for testing MBL App lifecycle Manager."""

import subprocess
import os
import sys

sys.path.append("/usr")
from bin.MBLAppLifecycleManager import AppLifecycleManager  # noqa

MBL_APP_MANAGER = "/usr/bin/mbl-app-manager"
MBL_APP_LIFECYCLE_MANAGER = "/usr/bin/MBLAppLifecycleManager.py"
IPK_TEST_FILE = "/app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk"
TEST_APP_ID = "app-lifecycle-manager-test-package"
CONTAINER_ID = "test_container_id_1"


def _run_app_manager(command):
    p = subprocess.Popen(
        command,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=-1,
    )
    output, error = p.communicate()
    assert p.returncode == 0


class TestMblAppLifecycleManager:
    """MBL App Lifecycle Manager main class."""

    @classmethod
    def setup_class(cls):
        """Setup any state specific to the execution of the given class."""
        # Install app_lifecycle_mng_test_container
        # All container operation will be done using it
        assert os.path.isfile(IPK_TEST_FILE), "Missing IPK test file."
        command = ["python3", MBL_APP_MANAGER, "-f", IPK_TEST_FILE]
        print("\nSetup: Installing test container package...")
        _run_app_manager(command)
        print("Installing test container package - done.\n")

    @classmethod
    def teardown_class(cls):
        """Teardown everything previously setup using a call to setup_class."""
        command = ["python3", MBL_APP_MANAGER, "-r", TEST_APP_ID]
        print("\nTeardown: Deleting test container package...")
        _run_app_manager(command)
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
        # Run operations
        app_lifecycle_mgr = AppLifecycleManager()
        self._run_container(CONTAINER_ID, TEST_APP_ID)
        assert app_lifecycle_mgr.container_running(CONTAINER_ID)
        # Stop container, when done - container should not exist anymore
        self._stop_container(CONTAINER_ID, timeout="10")
        assert not app_lifecycle_mgr.container_exists(CONTAINER_ID)

    def test_run_container_kill_container(self):
        """
        Test run container and kill container (stop is not called).

        :return: None
        """
        # Run operations
        app_lifecycle_mgr = AppLifecycleManager()
        self._run_container(CONTAINER_ID, TEST_APP_ID)
        assert app_lifecycle_mgr.container_running(CONTAINER_ID)
        # Stop container, when done - container should not exist anymore
        self._kill_container(CONTAINER_ID)
        assert not app_lifecycle_mgr.container_exists(CONTAINER_ID)

    def _run_container(self, CONTAINER_ID, application_id):
        # Run container
        command = [
            "python3",
            MBL_APP_LIFECYCLE_MANAGER,
            "-r",
            CONTAINER_ID,
            "-a",
            application_id,
        ]
        print("Executing command: " + " ".join(command))
        subprocess.run(command)

    def _stop_container(self, CONTAINER_ID, timeout):
        # Run container
        command = [
            "python3",
            MBL_APP_LIFECYCLE_MANAGER,
            "-s",
            CONTAINER_ID,
            "-t",
            timeout,
        ]
        print("Executing command: " + " ".join(command))
        subprocess.run(command)

    def _kill_container(self, CONTAINER_ID):
        # Run container
        command = ["python3", MBL_APP_LIFECYCLE_MANAGER, "-k", CONTAINER_ID]
        print("Executing command: " + " ".join(command))
        subprocess.run(command)
