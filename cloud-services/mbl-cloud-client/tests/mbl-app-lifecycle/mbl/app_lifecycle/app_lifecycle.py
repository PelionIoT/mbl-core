# !/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
app_lifecycle.py implements application start, stop and connectivity check.
"""

import sys
import os
import logging
from enum import Enum
from pydbus.generic import signal
from pydbus import SessionBus
from gi.repository import GLib

__version__ = "1.0"
DEFAULT_DBUS_NAME = "mbl.app.test1"


class ReturnCode(Enum):
    """AppLifeCycle error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1


class AppLifeCycle():
    """
        <node name='/mbl/app/test1'>
            <interface name='mbl.app.test1'>
                <method name='Stop'>
                    <arg type='n' name='result' direction='out'/>
                </method>
            </interface>
        </node>
    """

    # D-Bus main loop should be static, since it is used by different contexts
    bus_main_loop = GLib.MainLoop()

    def __init__(self):
        """Create AppLifeCycle object."""
        self.logger = logging.getLogger("mbl-app-lifecycle")
        self.logger.debug(
            "Creating AppLifeCycle version {}".format(__version__)
        )
        self.methods_for_publish_on_dbus = []

    def AddToPublish(self, dBusIfObjTuple):
        """Create AppLifeCycle object."""
        self.methods_for_publish_on_dbus.append(dBusIfObjTuple)
        self.logger.info(dBusIfObjTuple)

    def Start(self):
        """Start Main loop."""
        self.logger.info("Connect D-Bus: {}".format(DEFAULT_DBUS_NAME))

        bus = SessionBus()
        self.logger.info(
            "Publishing objects: {}".format(*(self.methods_for_publish_on_dbus))
        )
        self.obj = bus.publish(
            DEFAULT_DBUS_NAME, AppLifeCycle(),
            *(self.methods_for_publish_on_dbus)
        )

        self.logger.info("Running main loop...")
        type(self).bus_main_loop.run()

    def Stop(self):
        """Return whatever is passed to it."""
        self.logger.info("Quit the AppLifeCycle main loop")
        type(self).bus_main_loop.quit()
        self.logger.info("AFTER Quit the AppLifeCycle main loop")
        return 0



class AppConnectivity(AppLifeCycle):
    """
        <node name='/mbl/app/test1/AppConnectivity1'>
            <interface name='mbl.app.test1.AppConnectivity1'>
                <method name='Hello'>
                    <arg type='s' name='result' direction='out'/>
                </method>
            </interface>
        </node>
    """

    def __init__(self):
        """Create AppConnectivity object."""
        AppLifeCycle.__init__(self)
        self.logger.debug("__init__")

    def Start(self):
        """Publish metods on D-Bus and call super Start."""
        self.logger.info("AppConnectivity Start")
        AppLifeCycle.AddToPublish(
            self, dBusIfObjTuple=("AppConnectivity1", self)
        )
        AppLifeCycle.Start(self)

    def Hello(self):
        """Print Hello."""
        output = "Hello!"
        self.logger.info(output)
        print(output)
        return output
