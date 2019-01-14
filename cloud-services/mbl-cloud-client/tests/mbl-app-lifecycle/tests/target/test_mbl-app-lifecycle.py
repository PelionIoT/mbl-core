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
from enum import Enum
from pydbus.generic import signal
from pydbus import SessionBus
from gi.repository import GLib


DEFAULT_DBUS_NAME = "mbl.app.test1"
DBUS_OBJECT_PATH_APP_CONNECTIVITY1 = "/mbl/app/test1/AppConnectivity1"
DBUS_STOP_SIG = "mbl.app.test1.stop"

DBUS_SERVICE_PUBLISHING_TIME = 2
APP_LIFECYCLE_PROCESS_TERMINATION_TIMEOUT = 10


class TstReturnCode(Enum):
    """Test ReturnCode error codes."""

    TEST_SUCCESS = 0
    TEST_FAILED = 1


class TestAppConnectivity:
    """Test application connectivity main class."""

    """
    TestAppConnectivity definition.
    Emit / Publish a application stop signal
    """
    dbus = """
    <node>
        <interface name="mbl.app.test1.stop">
            <signal name="stop_signal">
            </signal>
        </interface>
    </node>
    """
    stop_signal = signal()

    def setup_method(self, method):
        """Setup any state specific to the execution of the given method."""
        print("Setup method TestAppConnectivity...")

        # get the session bus
        self.bus = SessionBus()
        self.obj = self.bus.publish(DBUS_STOP_SIG, self)

        command = ["mbl-app-lifecycle", "-v"]
        print("Executing command: {}".format(command))

        self.proc = subprocess.Popen(command)
        print("Finish executing command: {}".format(command))

        # time out to enable service to be published on the D-Bus
        time.sleep(DBUS_SERVICE_PUBLISHING_TIME)

        print("Setup method TestAppConnectivity end")

    def teardown_method(self, method):
        """Teardown method: stop the D-Bus main loop."""
        print("Teardown method TestAppConnectivity start...")

        print("Stopping mbl-app-lifecycle main loop")
        # send stop signal
        self.stop_signal()

        try:
            print("Waiting for App Pydbus process to terminate.")
            returncode = self.proc.wait(
                timeout=APP_LIFECYCLE_PROCESS_TERMINATION_TIMEOUT
            )
            assert returncode == 0

        except subprocess.TimeoutExpired:
            print("TimeoutExpired for process wait for termination, killing "
                "mbl-app-lifecycle process")
            self.proc.kill()
            out, err = self.proc.communicate()
            print("Process communicate output: {}, error: {}".format(out, err))
            return TstReturnCode.TEST_FAILED

        print("Teardown method TestAppConnectivity end")

    def test_app_hello(self):
        """Print Hello."""
        # get the object
        the_object = self.bus.get(
            DEFAULT_DBUS_NAME, object_path=DBUS_OBJECT_PATH_APP_CONNECTIVITY1
        )
        # call Hello method
        result = the_object.Hello()
        assert result == "Hello!"
        print("Hello method call on D-Bus returned: {}".format(result))
