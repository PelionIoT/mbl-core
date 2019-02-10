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
DBUS_STOP_SIGNAL_FILTER = "/com/test/Framework1"
APP_DBUS_NAME = "com.test.app"


class ReturnCode(Enum):
    """AppLifeCycle error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1


class AppLifeCycle:
    """Application life cycle."""

    dbus = """
<node name='/com/test/app'>
    <interface name='com.test.app.AppLifeCycle1'>
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

        # tuples of a path and a object to be published on bus. In order to
        # publish methods on bus, the inheriting class should call AddToPublish()
        # method and add path and a object touples to
        # all_methods_for_publish_on_dbus. Relative or absolute paths could be
        # used.
        self.all_methods_for_publish_on_dbus = []

    def AddToPublish(self, methods_for_publish_on_dbus):
        """Add object interfaces to the data to be published on D-Bus."""
        # Append interfaces to the list of all interfaces that will be published.
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
        self.logger.info("Connect the D-Bus bus")
        # get Session bus.
        bus = SessionBus()

        self.logger.debug(
            "Publishing following interfaces on D-Bus: {}".format(
                *(self.all_methods_for_publish_on_dbus)
            )
        )
        # publish() method could be used only once per a bus name. Multiple
        # objects are exported here by passing them in additional parameter
        # all_methods_for_publish_on_dbus. This parameter is composed of tuples
        # of a relative path and a object.
        self.obj = bus.publish(
            APP_DBUS_NAME + ".AppLifeCycle1",
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
        # Quit D-Bus bus main loop.
        AppLifeCycle.bus_main_loop.quit()


class AppConnectivity(AppLifeCycle):
    """Checks connectivity of caller to the application through the D-Bus."""

    dbus = """
<node name='/com/test/app/AppLifeCycle1/AppConnectivity1'>
    <interface name='com.test.app.AppLifeCycle1.AppConnectivity1'>
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
        # Add tuple of a path and a object to be published on bus. Publish
        # is performed by the parent class.
        AppLifeCycle.AddToPublish(
            self, methods_for_publish_on_dbus=("AppConnectivity1", self)
        )
        # Call publish() and run D-Bus bus main loop.
        AppLifeCycle.Run(self)

    def Hello(self):
        """
        Check D-Bus connectivity.

        :return: 'Hello!'
        """
        output = "Hello!"
        self.logger.info("{} request processed".format(output))
        return output
