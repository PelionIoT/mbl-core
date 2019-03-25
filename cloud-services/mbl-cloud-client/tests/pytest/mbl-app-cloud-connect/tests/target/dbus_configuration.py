#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pelion Connect pytests common components."""

# Prevent D-Bus binding to search the bus address
# on the X Window (X11) environment
DISPLAY = "0"

PELION_CONNECT_DBUS_BUS_ADDRESS = (
    "unix:path=/var/run/dbus/mbl_cloud_bus_socket"
)

# Pelion connect D-Bus components
PELION_CONNECT_DBUS_NAME = "com.mbed.Pelion1"
PELION_CONNECT_DBUS_OBJECT_PATH = "/com/mbed/Pelion1/Connect"
PELION_CONNECT_DBUS_INTERFACE_NAME = "com.mbed.Pelion1.Connect"
