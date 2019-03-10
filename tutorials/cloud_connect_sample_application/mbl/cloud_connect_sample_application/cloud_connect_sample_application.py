#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Sample application that demonstrates mbl application connectivity to Cloud Client."""

import os
import sys
import importlib
import subprocess
import time
import logging
from enum import Enum
from pydbus import SessionBus
from gi.repository import GLib

DBUS_MBL_CLOUD_BUS_ADDRESS = "unix:path=/var/run/dbus/mbl_cloud_bus_socket"

# find the existing bus address on the X display (X11) environment
DISPLAY = "0"

CLOUD_CONNECT_DBUS_NAME = "com.mbed.Cloud"
CLOUD_CONNECT_DBUS_OBJECT_PATH = (
    "/com/mbed/Cloud/Connect1"
)

logger = logging.getLogger("mbl-app-cloud-connect")

class CloudConnectSampleApp:
    """Cloud Connect sample application main class."""

    def setup(self):
        """Set up connection to D-Bus."""
        logger.info("Connecting to D-Bus {} D-Bus object {}...".format(
            CLOUD_CONNECT_DBUS_NAME, 
            CLOUD_CONNECT_DBUS_OBJECT_PATH
            )
        )

        os.environ["DISPLAY"] = DISPLAY
        os.environ["DBUS_SESSION_BUS_ADDRESS"] = DBUS_MBL_CLOUD_BUS_ADDRESS

        # get the session bus
        self.bus = SessionBus()

        try:
            self.cc_object = self.bus.get(
                CLOUD_CONNECT_DBUS_NAME,
                object_path=CLOUD_CONNECT_DBUS_OBJECT_PATH,
            )
        except GLib.GError:
            logger.error("Connection to the D-Bus {} D-Bus object {} failed!".format(
                CLOUD_CONNECT_DBUS_NAME, 
                CLOUD_CONNECT_DBUS_OBJECT_PATH
                )
            )
            

        assert self.cc_object, "Couldn't get {} D-Bus object".format(
            CLOUD_CONNECT_DBUS_OBJECT_PATH
        )
        
        logger.info("Connected successfully to the D-Bus {} D-Bus object {}...".format(
            CLOUD_CONNECT_DBUS_NAME, 
            CLOUD_CONNECT_DBUS_OBJECT_PATH
            )
        )

    def register_resources(self, json_data):
        """RegisterResources method."""

        logger.info("Calling RegisterResources with json_data = {}".format(json_data))

        result = self.cc_object.RegisterResources(json_data)
        
        assert (
            result[0] == 0
        ), "RegisterResources method call on D-Bus returned wrong value: {}".format(result[0])
        
        logger.info("RegisterResources method call on D-Bus returned: {}".format(result))

