#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Command line interface for mbl-app-manager."""

import argparse
import logging
import os
import sys
from enum import Enum

from .manager import AppManager
from .utils import log, set_log_verbosity


class ReturnCode(Enum):
    """Application return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


class CliActions:
    """Handle the command line interface commands."""

    @staticmethod
    def install_action(args):
        """Entry point for the 'install' cli command."""
        handler = AppManager()
        handler.install_app(args.app_package, args.app_path)

    @staticmethod
    def force_install_action(args):
        """Entry point for the 'force-install' cli command."""
        handler = AppManager()
        handler.force_install_app(args.app_package, args.app_path)

    @staticmethod
    def remove_action(args):
        """Entry point for the 'remove' cli command."""
        handler = AppManager()
        handler.remove_app(args.app_name, args.app_path)

    @staticmethod
    def list_action(args):
        """Entry point for the 'list' cli command."""
        handler = AppManager()
        handler.list_installed_apps(args.apps_path)


def parse_args():
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL application manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    command_group = parser.add_subparsers(
        description="The commands to control the application statuses."
    )

    install = command_group.add_parser(
        "install", help="install a user application."
    )
    install.add_argument(
        "app_package",
        type=str,
        help="application package to install a user application.",
    )
    install.add_argument(
        "-p",
        "--app-path",
        type=str,
        required=True,
        help="path to install the app.",
    )
    install.set_defaults(func=CliActions.install_action)

    force_install = command_group.add_parser(
        "force-install",
        help=(
            "remove a previous installation of a user"
            " application before installing."
        ),
    )
    force_install.add_argument(
        "app_package",
        type=str,
        help="application package to install a user application.",
    )
    force_install.add_argument(
        "-p",
        "--app-path",
        type=str,
        required=True,
        help="path to install the app.",
    )
    force_install.set_defaults(func=CliActions.force_install_action)

    remove = command_group.add_parser(
        "remove", help="remove a user application."
    )
    remove.add_argument(
        "app_name", type=str, help="name of the user application to remove."
    )
    remove.add_argument(
        "-p",
        "--app-path",
        type=str,
        required=True,
        help="path the app was installed.",
    )
    remove.set_defaults(func=CliActions.remove_action)

    lister = command_group.add_parser(
        "list", help="list installed applications."
    )
    lister.add_argument(
        "-p",
        "--apps-path",
        type=str,
        required=True,
        help="path to look for installed applications.",
    )
    lister.set_defaults(func=CliActions.list_action)

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="increase verbosity of status information",
    )

    args_namespace = parser.parse_args()

    # We want to fail gracefully, with a consistent
    # help message, in the no argument case.
    # So here's an obligatory hasattr hack.
    if not hasattr(args_namespace, "func"):
        parser.error("No arguments given!")
    else:
        return args_namespace


def run_mbl_app_manager():
    """Application main algorithm."""
    args = parse_args()

    set_log_verbosity(args.verbose)

    log.info("Starting mbl-app-manager")
    log.debug("Command line arguments:{}".format(args))

    args.func(args)

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
