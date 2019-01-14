# !/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""This script implements application start, stop and connectivity check."""

import sys
import os
import logging
from enum import Enum
from pydbus.generic import signal
from pydbus import SessionBus
from gi.repository import GLib

__version__ = "1.0"
DEFAULT_DBUS_NAME = "mbl.app.test1"
DBUS_STOP_SIG = "mbl.app.test1.stop"


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

    # D-Bus main loop should be static, since it is used by different contexts
    bus_main_loop = GLib.MainLoop()

    def __init__(self):
        """Create AppLifeCycle object."""
        self.logger = logging.getLogger("mbl-app-lifecycle")
        self.logger.info(
            "Creating AppLifeCycle version {}".format(__version__)
        )
        self.methods_for_publish_on_dbus = []

    def AddToPublish(self, dBusIfObjTuple):
        """Add object interfaces to the data to be published on D-Bus."""
        self.methods_for_publish_on_dbus.append(dBusIfObjTuple)
        self.logger.debug(dBusIfObjTuple)

    def Run(self):
        """Run Main loop."""
        self.logger.info("Connect D-Bus: {}".format(DEFAULT_DBUS_NAME))

        bus = SessionBus()
        self.logger.debug(
            "Publishing objects: {}".format(
                *(self.methods_for_publish_on_dbus)
            )
        )
        self.obj = bus.publish(
            DEFAULT_DBUS_NAME,
            AppLifeCycle(),
            *(self.methods_for_publish_on_dbus)
        )

        # subscribe for the Stop signal
        bus.subscribe(object=DBUS_STOP_SIG, signal_fired=self.StopSignal)

        self.logger.info("Running main loop...")
        type(self).bus_main_loop.run()

    def GetPid(self):
        """Test method: returns incremented input."""
        self.logger.debug("Application process ID {}".format(os.getpid()))
        return os.getpid()

    def StopSignal(self):
        """Return whatever is passed to it."""
        self.logger.info("Quit the AppLifeCycle main loop")
        print("***os.getpid() {}".format(os.getpid()))

        type(self).bus_main_loop.quit()


class AppConnectivity(AppLifeCycle):
    """Application connectivity."""

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
        """Publish metods on D-Bus and call super Run."""
        self.logger.info("AppConnectivity Run")
        AppLifeCycle.AddToPublish(
            self, dBusIfObjTuple=("AppConnectivity1", self)
        )
        AppLifeCycle.Run(self)

    def Hello(self):
        """Print Hello."""
        output = "Hello!"
        self.logger.info("{} request processed".format(output))
        return output
