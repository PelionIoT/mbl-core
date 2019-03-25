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
from dbus_configuration import PELION_CONNECT_DBUS_NAME
from dbus_configuration import PELION_CONNECT_DBUS_OBJECT_PATH
from dbus_configuration import PELION_CONNECT_DBUS_INTERFACE_NAME
from dbus_configuration import PELION_CONNECT_DBUS_BUS_ADDRESS
from dbus_configuration import DISPLAY


@pytest.mark.parametrize(
    "dbus_method_name,expected_result",
    [
        ("RegisterResources", {"in": ["s"], "out": ["u", "s"]}),
        ("SetResourcesValues", {"in": ["s", "a(syv)"], "out": ["au"]}),
        ("GetResourcesValues", {"in": ["s", "a(sy)"], "out": ["a(uyv)"]}),
    ],
)
class TestPelionConnectAPI:
    """Pelion Connect D-Bus API methods test."""

    # setup_method() called by pytest before each function test_*
    def setup_method(self, method):
        """Set up connection to D-Bus."""
        print(
            "Connecting to D-Bus {} D-Bus object {}...".format(
                PELION_CONNECT_DBUS_NAME, PELION_CONNECT_DBUS_OBJECT_PATH
            )
        )

        os.environ["DISPLAY"] = DISPLAY
        os.environ[
            "DBUS_SESSION_BUS_ADDRESS"
        ] = PELION_CONNECT_DBUS_BUS_ADDRESS

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
            self.cloud_connect_dbus_object = self.bus.get(
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
    def test_introspect(self, dbus_method_name, expected_result):
        """Verifies signatures of Pelion Connect D-Bus API methods."""

        print(
            "Verifying {} method in the {} interface...".format(
                dbus_method_name, PELION_CONNECT_DBUS_INTERFACE_NAME
            )
        )

        try:
            introspect_reply = self.cloud_connect_dbus_object.Introspect()
        except GLib.GError as glib_err:
            print(
                "Pelion Connect Introspect() failed. Error: {}!".format(
                    glib_err
                )
            )
            raise glib_err

        # parse XML data that was gotten from introspection
        root = ET.fromstring(introspect_reply)

        # verify existence of the Pelion interface under the 'root'
        pelion_interface = root.find(
            "./interface[@name='{}']".format(
                PELION_CONNECT_DBUS_INTERFACE_NAME
            )
        )

        assert (
            pelion_interface is not None
        ), "Couldn't find {} interface".format(
            PELION_CONNECT_DBUS_INTERFACE_NAME
        )

        # find method node
        method_node = pelion_interface.find(
            "./method[@name='{}']".format(dbus_method_name)
        )

        assert method_node is not None, "Couldn't find {} method".format(
            dbus_method_name
        )

        # for 'in' and 'out' directions,
        # collect actual signatures and verify against expected
        for direction in ["in", "out"]:
            actual_types_signatures = []
            # collect signatures for the arguments for the given
            # direction(in/out)
            for argument_signature in method_node.findall(
                "./arg[@direction='{}']".format(direction)
            ):
                actual_types_signatures.append(
                    argument_signature.attrib["type"]
                )

            # we compare expected signature to the actual assuming that
            # method_node.findall returns output in the same order, as
            # parameters are presented in XML data.
            assert expected_result[direction] == actual_types_signatures, (
                "Expected '{}' types '{}' for the method '{}' differs"
                + "from actual types '{}'!".format(
                    direction,
                    expected_result[direction],
                    dbus_method_name,
                    actual_types_signatures,
                )
            )

        print(
            "Method {} signature successfully verified.".format(
                dbus_method_name
            )
        )
