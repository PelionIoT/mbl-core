#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing mbl application connectivity to Cloud Client."""

import os
import sys
import importlib
import subprocess
import time
from pydbus import SessionBus
from gi.repository import GLib


proc = subprocess.Popen(["echo", "TestAppConnectivity"])
DEFAULT_DBUS_NAME = "mbl.app.test1"
object_path = "/mbl/app/test1/AppConnectivity1"

PROC_START_TIMEOUT_SEC = 2
PROC_TERMINATE_TIMEOUT_SEC = 10


class TestAppConnectivity:
    """Test application connectivity main class."""

    @classmethod
    def setup_class(cls):
        """Setup any state specific to the execution of the given class."""
        print("Setup TestAppConnectivity...")

        command = ["mbl-app-lifecycle", "-v"]
        print("Executing command: {}".format(command))

        proc = subprocess.Popen(command)

        # time out to enable service to be published on the D-Bus
        time.sleep(PROC_START_TIMEOUT_SEC)

        print("Finish executing command: {}".format(command))

        print("Setup TestAppConnectivity end")

    def setup_method(self, method):
        """Setup any state specific to the execution of the given method."""
        print("Setup method TestAppConnectivity...")
        # get the session bus
        self.bus = SessionBus()
        print("Setup method TestAppConnectivity end")

    @classmethod
    def teardown_class(cls):
        """Teardown everything previously setup using a call to setup_class."""
        print("Teardown TestAppConnectivity start...")

        try:
            print("Waiting for App Pydbus process to terminate.")
            returncode = proc.wait(timeout=PROC_TERMINATE_TIMEOUT_SEC)
            assert returncode == 0

        except subprocess.TimeoutExpired:
            print("TimeoutExpired for process wait for termination.")
            proc.kill()
            out, err = proc.communicate()
            print("Process communicate output: {}, error: {}".format(out, err))

        print("Teardown TestAppConnectivity end")

    def test_app_hello(self):
        """Print Hello."""
        # get the object
        the_object = self.bus.get(DEFAULT_DBUS_NAME, object_path=object_path)

        # call Hello method
        result = the_object.Hello()
        assert result == "Hello!"
        print("Hello method call on D-Bus returned: {}".format(result))

        return 0

    def test_app_quit_loop(self):
        """Quit the D-Bus main loop."""
        # get the object
        the_object = self.bus.get(DEFAULT_DBUS_NAME)

        # call Stop method
        result = the_object.Stop()
        assert result == 0

        return result
