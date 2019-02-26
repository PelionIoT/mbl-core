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
from pydbus import SessionBus
from gi.repository import GLib
import xml.etree.ElementTree as ET

# Prevent D-Bus binding to search the bus address
# on the X Window (X11) environment
DISPLAY = "0"

# Pelion connect D-Bus properties
PELION_CONNECT_DBUS_NAME = "com.mbed.Pelion1"
PELION_CONNECT_DBUS_OBJECT_PATH = "/com/mbed/Pelion1/Connect"


class IntrospectTest:
    """Introspect test."""

    logger = logging.getLogger("pytest-pelion-connect")

    def setup(self, dbus_session_bus_address):
        """Set up connection to D-Bus."""
        print(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

#        os.environ["DISPLAY"] = DISPLAY
#        os.environ["DBUS_SESSION_BUS_ADDRESS"] = dbus_session_bus_address

        # get the session bus
        try:
            self.bus = SessionBus()
        except GLib.GError as glib_err:
            print(
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
            print(
                "Couldn't connect to D-Bus {} object {}. Error: {}!".format(
                    PELION_CONNECT_DBUS_NAME,
                    PELION_CONNECT_DBUS_OBJECT_PATH,
                    glib_err,
                )
            )
            raise glib_err

        print(
            "Successfully connected to the D-Bus {} object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

    def sub_element_is_exist(self, parent_element, element_tag, element_attrib_name):
        elements = parent_element.findall(element_tag)
        for element in elements:
            #print(element)
            print(element.attrib['name'])
            
            if element.attrib['name'] == element_attrib_name:
                return element

        # required elemnt is not found
        raise KeyError("AAAAAAAAAAAAAAAAAAAa")
    
    

    def introspect_test(self, expected_dbus_api_methods):
        """Verifies existence of the expected D-Bus methods in the Pelion Connect D-Bus interface."""
        print(
            "Verifying {} methods...".format(
                expected_dbus_api_methods
            )
        )

        try:
            introspect_reply = self.cc_object.Introspect()
        except GLib.GError as glib_err:
            print(
                "Pelion Connect Introspect() failed. Error: {}!".format(
                    glib_err
                )
            )
            raise glib_err

        print(
            "Introspect method call returned: {}".format(introspect_reply)
        )
        
        root = ET.fromstring(introspect_reply)
        
        self.sub_element_is_exist(root, "interface", "com.mbed.Pelion1.Connect")

        
print("create IntrospectTest...")
app = IntrospectTest()

print("setup IntrospectTest...")
app.setup("unix:path=/var/run/dbus/mbl_cloud_bus_socket")

print("test IntrospectTest...")
app.introspect_test(0)


# replace print to print or print
