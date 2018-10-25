#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Firmware update manager library."""

import subprocess
import os
import logging
from enum import Enum

__version__ = "1.0"

ARM_UPDATE_ACTIVATE_SCRIPT = os.path.join(
    os.sep, "opt", "arm", "arm_update_activate.sh"
)
DUMMY_HEADER_FILE = os.path.join(os.sep, "scratch", "dummy_header_file")


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
        # Create an empty file.
        # Open a file with writing mode, and then just close it.
        with open(DUMMY_HEADER_FILE, "w"):
            pass
        command = [
            ARM_UPDATE_ACTIVATE_SCRIPT,
            "--firmware",
            firmware_update_file_path,
            "--header",
            DUMMY_HEADER_FILE,
        ]
        self.logger.debug("Executing command: {}".format(command))
        result = subprocess.run(
            command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
        os.remove(DUMMY_HEADER_FILE)
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
