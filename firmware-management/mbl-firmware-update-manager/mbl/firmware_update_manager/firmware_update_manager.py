#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Firmware update manager library."""

import subprocess
import os
import logging
from enum import Enum
import time

import mbl.firmware_update_header as mfuh

__version__ = "1.0"

ARM_UPDATE_ACTIVATE_SCRIPT = os.path.join(
    os.sep, "opt", "arm", "arm_update_activate.sh"
)
HEADER_FILE = os.path.join(os.sep, "scratch", "firmware_update_header_file")

def _create_header_data(payload_path):
    """
    Create update HEADER file data.

    This is a binary data file that arm_update_activate.sh expects to
    receive that contains information about the update. The only fields
    that make sense in this context are the firmware version and firmware hash
    fields.

    The firmware version is really a UNIX timestamp.
    """
    header = mfuh.FirmwareUpdateHeader()
    header.firmware_version = int(time.time())
    with open(payload_path, 'rb') as payload:
        header.firmware_hash = mfuh.calculate_firmware_hash(payload)
    return header.pack()

class Error(Enum):
    """FirmwareUpdateManager error codes."""

    SUCCESS = 0
    ERR_INVALID_ARGS = 1
    ERR_OPERATION_FAILED = 2


class FirmwareUpdateManager(object):
    """Firmware update manager class."""

    def __init__(self):
        """Initialize AppManager class."""
        self.logger = logging.getLogger("FirmwareUpdateManager")
        self.logger.info(
            "Creating FirmwareUpdateManager version {}".format(__version__)
        )

    def update_firmware(self, firmware_update_file_path, skip_reboot=False):
        """
        Update firmware and reboot.

        :param firmware_update_file_path: Update firmware tar file.
        :param skip_reboot: If True - skip reboot, else - reboot after update.
        :return:
                Error.SUCCESS
                Error.ERR_OPERATION_FAILED
        """
        # Create a "HEADER" file for the update - this is a blob that contains
        # information about the update
        with open(HEADER_FILE, "wb") as header_file:
            header_file.write(_create_header_data(firmware_update_file_path))

        command = [
            ARM_UPDATE_ACTIVATE_SCRIPT,
            "--firmware",
            firmware_update_file_path,
            "--header",
            HEADER_FILE,
        ]
        self.logger.debug("Executing command: {}".format(command))
        result = subprocess.run(
            command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
        os.remove(HEADER_FILE)
        output = ""
        if result.stdout:
            output = result.stdout.decode("utf-8")
        if result.returncode != 0:
            self.logger.error(
                "Command {} failed with status {} and output [{}]".format(
                    command, result.returncode, output
                )
            )
            return Error.ERR_OPERATION_FAILED
        if not skip_reboot:
            self.logger.info("Operation successful, rebooting device...")
            os.system("reboot")
        return Error.SUCCESS

