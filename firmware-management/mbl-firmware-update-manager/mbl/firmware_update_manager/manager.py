# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Firmware update manager library."""

import os
import subprocess
import time

import mbl.firmware_update_header_util as mfuh

from .utils import log


UPDATE_ACTIVATION_SHELL_SCRIPT = os.path.join(
    os.sep, "opt", "arm", "arm_update_activate.sh"
)
HEADER_FILE = os.path.join(os.sep, "scratch", "firmware_update_header_file")


class FirmwareUpdateManager:
    """
    Responsible for handling firmware package.

    An update package is a tar archive that can contain a rootfs or ipk(s).
    The firmware installation is delegated to a shell script which handles
    the installation of the content of the update package.
    """

    def __init__(self, update_pkg):
        """Create a firmware update package handler."""
        self.update_pkg = update_pkg
        self.header_data = None

    def install(self, reboot=False):
        """Install the firmware from the update package.

        The caller has the option to request a reboot if requested.
        """
        cmd = [
            UPDATE_ACTIVATION_SHELL_SCRIPT,
            "--firmware",
            self.update_pkg,
            "--header",
            HEADER_FILE,
        ]

        log.debug("Executing command: {}".format(cmd))

        try:
            subprocess.run(
                cmd,
                check=True,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except subprocess.CalledProcessError as error:
            msg = (
                "Failed to update firmware '{}' from update package '{}',"
                " error: '{}'".format(
                    HEADER_FILE,
                    self.update_pkg,
                    error.stdout.decode()
                    if error.stdout
                    else error.returncode,
                )
            )
            raise FmwInstallError(msg)
        finally:
            os.remove(HEADER_FILE)

        if reboot:
            log.info("Firmware installed, rebooting device...")
            os.system("reboot")

    def create_header_data(self):
        """
        Create update HEADER file data.

        This is a binary data file that arm_update_activate.sh expects to
        receive that contains information about the update. The only fields
        that make sense in this context are the firmware version and firmware
        hash fields.

        The firmware version is really a UNIX timestamp.
        """
        try:
            header = mfuh.FirmwareUpdateHeader()
            header.firmware_version = int(time.time())
            with open(self.update_pkg, "rb") as payload:
                header.firmware_hash = mfuh.calculate_firmware_hash(payload)
            self.header_data = header.pack()
        except IOError:
            raise HeaderFileError("IOError when creating HEADER file data")
        except mfuh.FormatError:
            raise HeaderFileError("FormatError when creating HEADER file data")

    def append_header_data_to_header_file(self):
        """Append a generate header data to the header file."""
        try:
            with open(HEADER_FILE, "wb") as header_file:
                header_file.write(self.header_data)
        except IOError:
            raise HeaderFileError(
                'Failed to write header data to header file "{}"'.format(
                    HEADER_FILE
                )
            )


class HeaderFileError(Exception):
    """An exception to report errors related to the firmware header file."""


class FmwInstallError(Exception):
    """An exception to report a failure to install the firmware."""
