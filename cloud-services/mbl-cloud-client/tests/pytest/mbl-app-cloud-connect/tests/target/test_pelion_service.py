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
PELION_CONNECT_DBUS_INTERFACE_NAME = "com.mbed.Pelion1.Connect"
DBUS_MBL_CLOUD_BUS_ADDRESS = "unix:path=/var/run/dbus/mbl_cloud_bus_socket"



class TestPelion:
    """Introspect test."""

    logger = logging.getLogger("pytest-pelion-connect")


    ###########################################################################
    # setup() and teardown() common methods
    ###########################################################################

    def setup_method(self, method):
        """Set up connection to D-Bus."""
        print(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

#        os.environ["DISPLAY"] = DISPLAY
#        os.environ["DBUS_SESSION_BUS_ADDRESS"] = DBUS_MBL_CLOUD_BUS_ADDRESS

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

    ###########################################################################
    # Introspection test
    ###########################################################################

    def test_introspect(self):
        """Verifies existence of the expected D-Bus methods 
        in the Pelion Connect D-Bus interface.
        """

        expected_dbus_signatures = {
            "RegisterResources" : {"in":["s"], "out":["u", "s"]}, 
            "SetResourcesValues" : {"in":["s","a(syv)"], "out":["au"]}, 
            "GetResourcesValues" : {"in":["s","a(sy)"], "out":["a(uyv)"]},
        }


        print(
            "Verifying {} methods in the {} interface...".format(
                expected_dbus_signatures.keys(),
                PELION_CONNECT_DBUS_INTERFACE_NAME
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

        #parse json data
        root = ET.fromstring(introspect_reply)

        # verify existence of the Pelion interface under the 'root'
        pelion_interface = root.find("./interface[@name='" + PELION_CONNECT_DBUS_INTERFACE_NAME + "']")
        
        assert pelion_interface is not None, "Couldn't find {} interface".format(
            PELION_CONNECT_DBUS_INTERFACE_NAME
        )

        for method_name in expected_dbus_signatures.keys():
            # verify existence of the required methods under the Pelion interface
            method_node = pelion_interface.find("./method[@name='" + method_name + "']")

            assert method_node is not None, "Couldn't find {} method".format(
                method_name
            )

            # verify signature of the method
            directions = ["in", "out"]

            # for input and output types, 
            # collect actual types of and verify against expected 
            for direction in directions:
                
                actual_types_signatures = []
                # get all arg's signatures for this method for given direction
                for arg_node in method_node.findall("./arg[@direction='" + direction + "']"):
                    actual_types_signatures.append(arg_node.attrib['type'])

                assert expected_dbus_signatures[method_name][direction] == actual_types_signatures, "Expected {} types {} for the method {} differs from actual types {}!".format(
                        direction,
                        expected_dbus_signatures[method_name][direction],
                        method_name,
                        actual_types_signatures
                    )

# replace print to log.error or log.info