#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Command line interface for mbl-app-update-manager."""

import argparse
import logging
import os
import sys
from enum import Enum

from .manager import AppUpdateManager
from .utils import log, set_log_verbosity, __version__


class ReturnCode(Enum):
    """Application return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


def parse_args():
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL application update manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        "update_package",
        metavar="<update-package>",
        type=str,
        help="update package containing app(s) to install",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="increase verbosity of status information",
    )

    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s {}".format(__version__),
        help="print application version",
    )

    return parser.parse_args()


def run_mbl_app_update_manager():
    """Application main algorithm."""
    args = parse_args()

    set_log_verbosity(args.verbose)

    log.info("Starting mbl-app-update-manager")
    log.debug("Command line arguments:{}".format(args))

    handler = AppUpdateManager(args.update_package)

    if handler.unpack():
        if handler.install_apps():
            handler.start_installed_apps()


def _main():
    """Run mbl-app-update-manager."""
    try:
        sys.exit(run_mbl_app_update_manager())
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
