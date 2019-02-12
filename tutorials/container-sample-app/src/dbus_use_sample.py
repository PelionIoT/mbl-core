#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""D-Bus usage sample application."""

import os
import sys
import importlib
import subprocess
from pydbus import SessionBus
from gi.repository import GLib
from subprocess import *


DBUS_MBL_CLOUD_BUS_ADDRESS = "unix:path=/var/run/dbus/mbl_cloud_bus_socket"
# find the existing bus address on the X display (X11) environment
DISPLAY = "0"


def main():
    print("main start")

    os.environ["DISPLAY"] = DISPLAY
    os.environ["DBUS_SESSION_BUS_ADDRESS"] = DBUS_MBL_CLOUD_BUS_ADDRESS
    bus = SessionBus()

    command = ['dbus-send', BUS, \
             '--dest=org.freedesktop.DBus', \
             '--type=method_call', \
             '--print-reply', \
             '/org/freedesktop/DBus', \
             'org.freedesktop.DBus.ListNames' \
    ]

    #p = Popen(command, shell=True, stdout=PIPE)
    #while True:
    #  data = p.stdout.readline()   # Alternatively proc.stdout.read(1024)
    #  if len(data) == 0:
    #      break
    #  sys.stdout.write(data)


if __name__== "__main__":
  main()
