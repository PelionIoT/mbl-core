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
import time

__version__ = "1.0"
DEFAULT_DBUS_NAME = "mbl.app.test1"


class Error(Enum):
    """AppLifeCycle error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1


class AppLifeCycle(object):
    """
        <node name='/mbl/app/test1'>
            <interface name='mbl.app.test1'>
                <method name='Stop'>
                    <arg type='n' name='result' direction='out'/>
                </method>
            </interface>
        </node>
    """

    # main loop should be static, since it is used by different contexts
    loop = GLib.MainLoop()

    def __init__(self):
        """Create AppLifeCycle object."""
        self.logger = logging.getLogger("mbl-app-lifecycle")
        self.logger.info(
            "Creating AppLifeCycle version {}".format(__version__)
        )
        self.publish_tuples_list = []

    def AddToPublish(self, aTuple):
        """Create AppLifeCycle object."""
        self.publish_tuples_list.append(aTuple)
        print(aTuple)

    def Start(self):
        """Start Main loop."""
        self.logger.info("Connect D-Bus: {}".format(DEFAULT_DBUS_NAME))

        bus = SessionBus()
        self.logger.info(
            "Publishing objects: {}".format(*(self.publish_tuples_list))
        )
        self.obj = bus.publish(
            DEFAULT_DBUS_NAME, AppLifeCycle(), *(self.publish_tuples_list)
        )

        self.logger.info("Running main loop...")
        type(self).loop.run()
        return SUCCESS

    def Stop(self):
        """Return whatever is passed to it."""
        self.logger.info("Quit the AppLifeCycle main loop")
        type(self).loop.quit()
        self.logger.info("AFTER Quit the AppLifeCycle main loop")
        return 0

    def __del__(self):
        """Delete."""
        self.logger.debug("__del__")


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
        AppLifeCycle.AddToPublish(self, aTuple=("AppConnectivity1", self))
        AppLifeCycle.Start(self)

    def Hello(self):
        """Print Hello."""
        self.logger.info("Hello!")
        print("Hello!")
        return "Hello!"

    def __del__(self):
        """Delete."""
