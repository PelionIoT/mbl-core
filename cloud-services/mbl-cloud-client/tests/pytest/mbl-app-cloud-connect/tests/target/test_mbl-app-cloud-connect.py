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


DBUS_MBL_CLOUD_BUS_ADDRESS = "unix:path=/var/run/dbus/mbl_cloud_bus_socket"

# find the existing bus address on the X display (X11) environment
DISPLAY = "0"

APPLICATION_DBUS_NAME = "com.test.app.AppLifeCycle1"
DBUS_OBJECT_PATH_APP_CONNECTIVITY = (
    "/com/test/app/AppLifeCycle1/AppConnectivity1"
)
DBUS_STOP_SIGNAL = "com.test.Framework1"

DBUS_SERVICE_PUBLISHING_TIME = 1
DBUS_SERVICE_PUBLISHING_WAIT_MAX_RETRIES = 15
APP_PROCESS_TERMINATION_TIMEOUT = 15


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
        <interface name="com.test.Framework1">
            <signal name="stop_request_signal">
            </signal>
        </interface>
    </node>
    """

    stop_request_signal = signal()

    def setup_method(self, method):
        """Set up any state specific to the execution of the given method."""
        print("Setup method TestAppConnectivity...")

        os.environ["DISPLAY"] = DISPLAY
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = DBUS_MBL_CLOUD_BUS_ADDRESS
        # get the session bus
        self.bus = SessionBus()
        # publish stop signal
        self.obj = self.bus.publish(DBUS_STOP_SIGNAL, self)

        command = ["mbl-app-cloud-connect", "-v"]
        print("Executing command: {}".format(command))
        # run the application
        self.proc = subprocess.Popen(command)
        print("Finish executing command: {}".format(command))

        # time out to enable service to be published on the D-Bus
        for timeout in range(DBUS_SERVICE_PUBLISHING_WAIT_MAX_RETRIES):
            try:
                self.app_connectivity_obj = self.bus.get(
                    APPLICATION_DBUS_NAME,
                    object_path=DBUS_OBJECT_PATH_APP_CONNECTIVITY,
                )
                break
            except GLib.GError:
                print(
                    "Waiting for dbus object publication {} seconds".format(
                        timeout
                    )
                )
                time.sleep(DBUS_SERVICE_PUBLISHING_TIME)

        assert self.app_connectivity_obj, "Couldn't get {} DBus obj".format(
            DBUS_OBJECT_PATH_APP_CONNECTIVITY
        )
        print("Setup method TestAppConnectivity end")

    def teardown_method(self, method):
        """Teardown method: stop the D-Bus main loop."""
        print("Teardown method TestAppConnectivity start...")
        print("Stopping mbl-app-cloud-connect main loop")
        # send stop signal
        self.stop_request_signal()

        try:
            print("Waiting for App Pydbus process to terminate.")
            returncode = self.proc.wait(
                timeout=APP_PROCESS_TERMINATION_TIMEOUT
            )
            assert (
                returncode == 0
            ), "Wait for App Pydbus process to terminate returned code {} is"
            " not 0".format(returncode)

            print("Process wait return code: {}".format(returncode))

        except subprocess.TimeoutExpired:
            print(
                "TimeoutExpired for process wait for termination, killing "
                "mbl-app-cloud-connect process"
            )
            # timeout expired, kill the process
            self.proc.kill()
            out_app_cloud_connect, err_app_cloud_connect = (
                self.proc.communicate()
            )
            print(
                "Process communicate stdout: {}, stderr: {}".format(
                    out_app_cloud_connect, err_app_cloud_connect
                )
            )
            assert 0, "Wait for process terminate: TimeoutExpired"

        print("Teardown method TestAppConnectivity end")

    def test_app_hello(self):
        """Connectivity test: through the D-Bus call Hello method."""
        # call Hello method
        result = self.app_connectivity_obj.Hello()
        assert (
            result == "Hello!"
        ), "Hello method call on D-Bus returned wrong value: {}".format(result)
        print("Hello method call on D-Bus returned: {}".format(result))
