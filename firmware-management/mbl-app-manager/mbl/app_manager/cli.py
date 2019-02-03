#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Simple command line interface for mbl app manager."""

import argparse
import logging
import os
import sys
from enum import Enum

from .manager import AppManager
from .utils import log


APPS_INSTALLATION_PATH = "/home/app"


class ReturnCode(Enum):
    """Application return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


def parse_args():
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL application manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        usage="mbl-app-manager [arguments] [<file>|<app>]",
    )

    group = parser.add_mutually_exclusive_group(required=True)

    group.add_argument(
        "-i",
        "--install-app",
        metavar="<file>",
        type=str,
        help="package of the application to install.",
    )

    group.add_argument(
        "-f",
        "--force-install-app",
        metavar="<file>",
        type=str,
        help="remove an application if previously installed before installing",
    )

    group.add_argument(
        "-r",
        "--remove-app",
        metavar="<app>",
        type=str,
        help=(
            "remove installed application"
            " where <app> is the name of the application"
        ),
    )

    group.add_argument(
        "-l",
        "--list-installed-apps",
        action="store_true",
        help="list installed applications.",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print application status information",
    )

    return parser.parse_args()


def run_mbl_app_manager():
    """Application main algorithm."""
    args = parse_args()

    log.setLevel(logging.DEBUG if args.verbose else logging.INFO)

    log.info("Starting mbl-app-manager")
    logger.debug("Command line arguments:{}".format(args))

    handler = AppManager()

    if args.install_app:
        handler.install_app(
            args.install_app,
            os.path.join(
                APPS_INSTALLATION_PATH, handler.get_app_name(args.install_app)
            ),
        )
    elif args.remove_app:
        handler.remove_app(
            args.remove_app,
            os.path.join(APPS_INSTALLATION_PATH, args.remove_app),
        )
    elif args.force_install_app:
        handler.force_install_app(
            args.force_install_app,
            os.path.join(
                APPS_INSTALLATION_PATH,
                handler.get_app_name(args.force_install_app),
            ),
        )
    elif args.list_installed_packages:
        handler.list_installed_apps(APPS_INSTALLATION_PATH)

    return ReturnCode.SUCCESS.value


def _main():
    """Run mbl-app-manager."""
    try:
        sys.exit(run_mbl_app_manager())
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
