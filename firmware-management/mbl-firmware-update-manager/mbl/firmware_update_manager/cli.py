# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Simple command line interface for mbl firmware update manager."""

import argparse
import logging
import os
import sys
from enum import Enum

from .manager import FmwUpdateManager
from .utils import log, set_log_verbosity


class ReturnCode(Enum):
    """Application return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


def parse_args():
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL firmware update manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "update_package",
        metavar="<update-package>",
        type=str,
        help="update package containing firmware to install",
    )

    parser.add_argument(
        "-r",
        "--reboot",
        action="store_true",
        help="reboot after firmware update",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Increase output verbosity",
    )

    return parser.parse_args()


def run_mbl_firmware_update_manager():
    """Application main algorithm."""
    args = parse_args()

    set_log_verbosity(args.verbose)

    log.info("Starting mbl-firmware-update-manager")
    log.debug("Command line arguments:{}".format(args))

    handler = FmwUpdateManager(args.update_package)
    handler.create_header_data()
    handler.append_header_data_to_header_file()
    handler.install(args.reboot)


def _main():
    """Run mbl-firmware-update-manager."""
    try:
        run_mbl_firmware_update_manager()
    except Exception as error:
        print(error)
        return ReturnCode.ERROR.value
    else:
        return ReturnCode.SUCCESS.value


class ArgumentParserWithDefaultHelp(argparse.ArgumentParser):
    """Subclass that always shows the help message on invalid arguments."""

    def error(self, message):
        """Error handler."""
        sys.stderr.write("error: {}".format(message))
        self.print_help()
        raise SystemExit(ReturnCode.INVALID_OPTIONS.value)
