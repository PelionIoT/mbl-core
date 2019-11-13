#!/usr/bin/env python3

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Send AT commands to the cellular module and check for responses."""

import sys
import serial
import argparse
import time


def main():
    """Perform the main execution."""
    returnValue = 1

    # Parse command line
    options = setup_parser().parse_args()

    if options.command == "comms":
        returnValue = CellularModule.comms_check()
    elif options.command == "status":
        returnValue = CellularModule.status()
    elif options.command == "activate":
        returnValue = CellularModule.set_state(True)
    elif options.command == "deactivate":
        returnValue = CellularModule.set_state(False)
    elif options.command == "power":
        CellularModule.power_cycle()
    else:
        print("Unrecognised command {}".format(options.command))

    return returnValue


def setup_parser():
    """Create command line parser.

    :return: parser object.

    """
    parser = argparse.ArgumentParser(
        description=" Cellular Module Configuration"
    )
    parser = argparse.ArgumentParser(
        description=" Cellular Module Configuration"
    )

    parser.add_argument(
        "command",
        help="",
        choices=["comms", "status", "activate", "deactivate", "power"],
    )
    return parser


class CellularModule:
    """Class to encapsulate the comms to the cellular module."""

    def comms_check():
        """Check basic comms are functional."""
        returnValue = False
        ser = CellularModule._init_serial_port()
        if ser:
            ser.write(b"at\r")
            # Wait up to 5 seconds looking for "OK"
            for i in range(5):
                reply = ser.readline()
                if reply.find(b"OK\r\n") != -1:
                    print("Basic comms check to cellular module OK")
                    returnValue = True
                    break
            ser.close()
        return returnValue

    def status():
        """Print the status of the cellular module."""
        returnValue = False
        ser = CellularModule._init_serial_port()
        if ser:
            ser.write(b'at+qcfg="usbnet"\r')
            # Wait up to 5 seconds looking for "response"
            for i in range(5):
                reply = ser.readline()
                if reply.find(b'+QCFG: "usbnet",0\r') != -1:
                    print("Cellular Module is Disabled")
                    returnValue = True
                    break
                elif reply.find(b'+QCFG: "usbnet",1\r') != -1:
                    print("Cellular Module is Enabled")
                    returnValue = True
                    break
            ser.close()
        return returnValue

    def set_state(on):
        """Set the status of the cellular module."""
        returnValue = False

        print("Setting Cellular Module to ", end="")
        print("Enabled" if on else "Disabled", end=". ")

        ser = CellularModule._init_serial_port()
        if ser:
            if on:
                ser.write(b'at+qcfg="usbnet",1\r')
            else:
                ser.write(b'at+qcfg="usbnet",0\r')

            # Wait up to 5 seconds looking for "OK"
            for i in range(5):
                reply = ser.readline()
                if reply.find(b"OK\r\n") != -1:
                    print("Command received OK.")
                    returnValue = True
                    break
            ser.close()
        return returnValue

    def power_cycle():
        """Power cycle the cellular module."""
        returnValue = False
        ser = CellularModule._init_serial_port()
        print("Power cycling Cellular Module.", end="")
        if ser:
            ser.write(b"at+qpowd\r")

            # Wait up to 10 seconds looking for "POWERED DOWN"
            for i in range(10):
                time.sleep(1)
                print(".", end="", flush=True)
                reply = ser.readline()
                if reply.find(b"POWERED DOWN\r\n") != -1:
                    print("POWERED DOWN OK")
                    break

            ser.close()

            # Wait for the cellular module to reset.
            # Note: tried to do this using serial port exceptions
            # but it was not reliable.

            print("Waiting for the module to power up.", end="")
            for i in range(30):
                time.sleep(1)
                print(".", end="", flush=True)

            # Wait up to 5 seconds for the port to become available. It should
            # already be available inside the 30 second delay above.
            for i in range(5):
                try:
                    ser.open()
                    returnValue = CellularModule.comms_check()
                    break
                except serial.serialutil.SerialException as e:
                    time.sleep(1)
                    print(".", end="", flush=True)
            if ser:
                ser.close()
        return returnValue

    def _init_serial_port():
        """Initialise the serial port."""
        ser = serial.Serial()

        ser.baudrate = 9600
        ser.bytesize = 8
        ser.parity = "N"
        ser.stopbits = 1
        ser.timeout = 1
        ser.xonxoff = False
        ser.rtscts = False
        ser.dsrdtr = False
        ser.port = "/dev/ttyUSB2"

        try:
            ser.open()
        except serial.serialutil.SerialException as e:
            print("Failed to open serial port {}".format(ser.port))
            ser = None
        return ser


if __name__ == "__main__":
    sys.exit(main())
