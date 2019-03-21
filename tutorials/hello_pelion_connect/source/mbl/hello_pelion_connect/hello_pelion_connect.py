#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Hello-world Pelion Connect application."""

import os
import sys
import importlib
import subprocess
import time
import logging
from enum import Enum
from pydbus import SessionBus
from gi.repository import GLib

# Prevent D-Bus binding to search the bus address
# on the X Window (X11) environment
DISPLAY = "0"

# Pelion connect D-Bus properties
PELION_CONNECT_DBUS_NAME = "com.mbed.Pelion1"
PELION_CONNECT_DBUS_OBJECT_PATH = "/com/mbed/Pelion1/Connect"


class HelloPelionConnect:
    """Hello Pelion Connect application main class."""

    def __init__(self):
        """Construct the object."""
        self.logger = logging.getLogger("hello-pelion-connect")

    def setup(self, dbus_session_bus_address):
        """Set up connection to D-Bus."""
        self.logger.info(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

        os.environ["DISPLAY"] = DISPLAY
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = dbus_session_bus_address

        # get the session bus
        try:
            self.bus = SessionBus()
        except GLib.GError as glib_err:
            self.logger.error(
                "Couldn't get Pelion Connect D-Bus bus. Error: {}!".format(
                    glib_err
                )
            )
            raise glib_err

        # connect to the bus
        try:
            self.cc_object = self.bus.get(
                PELION_CONNECT_DBUS_NAME,
                object_path=PELION_CONNECT_DBUS_OBJECT_PATH,
            )
        except GLib.GError as glib_err:
            self.logger.error(
                "Couldn't connect to D-Bus {} object {}. Error: {}!".format(
                    PELION_CONNECT_DBUS_NAME,
                    PELION_CONNECT_DBUS_OBJECT_PATH,
                    glib_err,
                )
            )
            raise glib_err

        self.logger.info(
            "Successfully connected to the D-Bus {} object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

    def register_resources(self, app_resource_definition_file_path):
        """Register resources method."""
        self.logger.info(
            "Calling RegisterResources with json file path = {}".format(
                app_resource_definition_file_path
            )
        )

        with open(app_resource_definition_file_path, "r") as app_file:
            app_resource_definition_data = app_file.read()

        try:
            reg_reply = self.cc_object.RegisterResources(
                app_resource_definition_data
            )
        except GLib.GError as glib_err:
            self.logger.error(
                "Pelion Connect RegisterResources failed. Error: {}!".format(
                    glib_err
                )
            )
            raise glib_err

        self.logger.info(
            "RegisterResources method call returned: {}".format(reg_reply)
        )
