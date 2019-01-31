# !/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
D-Bus test application infrastructure script.

This script simulate application behavior for tests: application start, stop
and D-Bus connectivity check.
"""

import sys
import os
import logging
from enum import Enum
from pydbus.generic import signal
from pydbus import SessionBus
from gi.repository import GLib


__version__ = "1.0"
DBUS_STOP_SIGNAL_FILTER = "/mbl/app/test1/stop"
DEFAULT_DBUS_NAME = "mbl.app.test1"


class ReturnCode(Enum):
    """AppLifeCycle error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1


class AppLifeCycle:
    """Application life cycle."""

    dbus = """
<node name='/mbl/app/test1'>
    <interface name='mbl.app.test1'>
        <method name='GetPid'>
            <arg type='n' name='pid' direction='out'/>
        </method>
    </interface>
</node>
    """

    bus_main_loop = GLib.MainLoop()

    def __init__(self):
        """Create AppLifeCycle object."""
        self.logger = logging.getLogger("mbl-app-lifecycle")
        self.logger.info(
            "Creating AppLifeCycle version {}".format(__version__)
        )
        self.all_methods_for_publish_on_dbus = []

    def AddToPublish(self, methods_for_publish_on_dbus):
        """Add object interfaces to the data to be published on D-Bus."""
        self.all_methods_for_publish_on_dbus.append(
            methods_for_publish_on_dbus
        )
        self.logger.debug(
            "The following interfaces will be published on D-Bus: {}".format(
                methods_for_publish_on_dbus
            )
        )

    def Run(self):
        """Run Main loop."""
        self.logger.info("Connect D-Bus: {}".format(DEFAULT_DBUS_NAME))

        bus = SessionBus()
        self.logger.debug(
            "Publishing following interfaces on D-Bus: {}".format(
                *(self.all_methods_for_publish_on_dbus)
            )
        )
        self.obj = bus.publish(
            DEFAULT_DBUS_NAME,
            AppLifeCycle(),
            *(self.all_methods_for_publish_on_dbus)
        )

        # subscribe to the Stop signal:
        # object - object path to match on or None to match on all object
        # paths; signal_fired - callable, invoked when there is a signal
        # matching the requested data. For more information about subscribe()
        # method please refer Github:
        # https://github.com/LEW21/pydbus/blob/master/pydbus/subscription.py
        bus.subscribe(
            object=DBUS_STOP_SIGNAL_FILTER, signal_fired=self.StopSignal
        )

        self.logger.info("Running main loop...")
        AppLifeCycle.bus_main_loop.run()

    def GetPid(self):
        """
        Get process Id D-Bus test method.

        :return: process Id
        """
        pid = os.getpid()
        self.logger.debug("Application process ID {}".format(pid))
        return pid

    def StopSignal(self, *args):
        """Stop D-Bus main loop."""
        self.logger.info("Got stop signal, quitting the D-Bus main loop")
        AppLifeCycle.bus_main_loop.quit()


class AppConnectivity(AppLifeCycle):
    """Checks connectivity of caller to the application through the D-Bus."""

    dbus = """
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

    def Run(self):
        """Publish methods on D-Bus and run D-Bus main loop."""
        self.logger.info("AppConnectivity Run")
        AppLifeCycle.AddToPublish(
            self, methods_for_publish_on_dbus=("AppConnectivity1", self)
        )
        AppLifeCycle.Run(self)

    def Hello(self):
        """
        Check D-Bus connectivity.

        :return: 'Hello!'
        """
        output = "Hello!"
        self.logger.info("{} request processed".format(output))
        return output
