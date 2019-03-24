#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pelion Connect service end to end tests."""

import logging
import os
from pydbus import SessionBus
from gi.repository import GLib
import xml.etree.ElementTree as ET
import pytest
from pytests_utils import PELION_CONNECT_DBUS_NAME
from pytests_utils import PELION_CONNECT_DBUS_OBJECT_PATH
from pytests_utils import PELION_CONNECT_DBUS_INTERFACE_NAME
from pytests_utils import PELION_CONNECT_DBUS_SESSION_BUS_ADDRESS
from pytests_utils import DISPLAY

@pytest.mark.parametrize("method_name,expected_result", [
        ("RegisterResources", {"in": ["s"], "out": ["u", "s"] } ),
        ("SetResourcesValues", {"in": ["s","a(syv)"], "out": ["au"]}),
        ("GetResourcesValues", {"in": ["s","a(sy)"], "out": ["a(uyv)"]}) 
    ])

class TestPelionConnectAPI:
    """Pelion Connect D-Bus API methods test."""

    logger = logging.getLogger("pytest-pelion-connect")

    ###########################################################################
    # setup() and teardown() common methods
    ###########################################################################

    def setup_method(self, method):
        """Set up connection to D-Bus."""
        self.logger.info(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

#        os.environ["DISPLAY"] = DISPLAY
#        os.environ["DBUS_SESSION_BUS_ADDRESS"] = PELION_CONNECT_DBUS_SESSION_BUS_ADDRESS

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

    ###########################################################################
    # Introspection test
    ###########################################################################

    def test_introspect(self, method_name, expected_result):
        """Verifies signatures of Pelion Connect D-Bus API."""

        self.logger.info(
            "Verifying {} method in the {} interface...".format(
                method_name,
                PELION_CONNECT_DBUS_INTERFACE_NAME
            )
        )

        try:
            introspect_reply = self.cc_object.Introspect()
        except GLib.GError as glib_err:
            self.logger.error(
                "Pelion Connect Introspect() failed. Error: {}!".format(
                    glib_err
                )
            )
            raise glib_err

        #parse xml data gotten from introspection
        root = ET.fromstring(introspect_reply)

        # verify existence of the Pelion interface under the 'root'
        pelion_interface = root.find("./interface[@name='" + PELION_CONNECT_DBUS_INTERFACE_NAME + "']")
        
        assert pelion_interface is not None, "Couldn't find {} interface".format(
            PELION_CONNECT_DBUS_INTERFACE_NAME
        )

        # find method node
        method_node = pelion_interface.find("./method[@name='" + method_name + "']")

        assert method_node is not None, "Couldn't find {} method".format(
            method_name
        )

        # for 'in' and 'out' directions, 
        # collect actual signatures and verify against expected 
        for direction in ["in", "out"]:
            
            actual_types_signatures = []
            # collect arg's signatures for the given direction ('in' or 'out')
            for arg_node in method_node.findall("./arg[@direction='" + direction + "']"):
                actual_types_signatures.append(arg_node.attrib['type'])

            # we compare expected signature to the actual assuming that 
            # method_node.findall returns output in the same order they are
            # presented in xml data
            assert expected_result[direction] == actual_types_signatures, "Expected {} types {} for the method {} differs from actual types {}!".format(
                    direction,
                    expected_result[direction],
                    method_name,
                    actual_types_signatures
                )