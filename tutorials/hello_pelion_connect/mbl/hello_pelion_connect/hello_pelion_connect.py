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

# tell D-Bus binding (gd-bus) not to search the bus address
# on the X display (X11) environment
DISPLAY = "0"

# tell D-Bus binding (gd-bus) what is the bus address
# (which is unix socket path)
DBUS_MBL_PELION_CONNECT_BUS_ADDRESS = (
    "unix:path=/var/run/dbus/mbl_cloud_bus_socket"
)

PELION_CONNECT_DBUS_NAME = "com.mbed.Cloud"
PELION_CONNECT_DBUS_OBJECT_PATH = "/com/mbed/Cloud/Connect1"


class HelloPelionConnect:
    """Hello Pelion Connect application main class."""

    logger = logging.getLogger("hello-pelion-connect")

    def setup(self):
        """Set up connection to D-Bus."""
        self.logger.info(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

        os.environ["DISPLAY"] = DISPLAY
        os.environ[
            "DBUS_SESSION_BUS_ADDRESS"
        ] = DBUS_MBL_PELION_CONNECT_BUS_ADDRESS

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

    def register_resources(self, json_file_path):
        """Register resources method."""
        self.logger.info(
            "Calling RegisterResources with json file path = {}".format(
                json_file_path
            )
        )

        with open(json_file_path, "r") as json_file:
            json_data = json_file.read()

        try:
            reg_reply = self.cc_object.RegisterResources(json_data)
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
