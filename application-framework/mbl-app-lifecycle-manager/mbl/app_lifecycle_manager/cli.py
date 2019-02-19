# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Command line interface for mbl-app-lifecycle-manager."""

import argparse
import logging
import os
import shutil
import sys
from enum import Enum

from .container import OCI_BUNDLE_CONFIGURATION, OCI_BUNDLE_FILESYSTEM

from .manager import (
    run_app,
    terminate_app,
    kill_app,
    DEFAULT_TIMEOUT_AFTER_SIGTERM,
    DEFAULT_TIMEOUT_AFTER_SIGKILL,
)
from .utils import log, set_log_verbosity, human_sort


class ReturnCode(Enum):
    """Application return codes."""

    SUCCESS = 0
    ERROR = 1
    INVALID_OPTIONS = 2


def run_action(args):
    """Entry point for the 'run' cli command."""
    if all(
        x in os.listdir(args.app_path)
        for x in [OCI_BUNDLE_CONFIGURATION, OCI_BUNDLE_FILESYSTEM]
    ):
        run_app(args.app_name, args.app_path)
    else:
        app_version_dirs = []
        for dirpath, dirnames, filenames in os.walk(args.app_path):
            if (
                OCI_BUNDLE_FILESYSTEM in dirnames
                and OCI_BUNDLE_CONFIGURATION in filenames
            ):
                app_version_dirs.append(dirpath)

        if app_version_dirs:
            human_sort(app_version_dirs)
            app_path = app_version_dirs.pop(0)
            run_app(args.app_name, app_path)

            # clean up app versions
            for version in app_version_dirs:
                shutil.rmtree(version)


def terminate_action(args):
    """Entry point for the 'terminate' cli command."""
    terminate_app(args.app_name, args.sigterm_timeout, args.sigkill_timeout)


def kill_action(args):
    """Entry point for the 'kill' cli command."""
    kill_app(args.app_name, args.sigkill_timeout)


def parse_args():
    """Parse the command line args."""
    parser = ArgumentParserWithDefaultHelp(
        description="MBL application lifecycle manager",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    command_group = parser.add_subparsers(
        description="The commands to control the application statuses."
    )

    run = command_group.add_parser("run", help="run a user application.")
    run.add_argument(
        "app_name",
        type=str,
        help="name the application will be referred as"
        " once it has been started.",
    )
    run.add_argument(
        "app_path", type=str, help="path of the application to start."
    )

    run.set_defaults(func=run_action)

    terminate = command_group.add_parser(
        "terminate",
        help=(
            "kill a user application process with SIGTERM then delete its"
            " associated resources. Note: A SIGKILL is sent if the application"
            " does not stop after a SIGTERM."
        ),
    )
    terminate.add_argument(
        "app_name", type=str, help="name of the application to terminate."
    )
    terminate.add_argument(
        "-t",
        "--sigterm-timeout",
        type=int,
        default=DEFAULT_TIMEOUT_AFTER_SIGTERM,
        help="timeout to wait for an application to stop after SIGTERM.",
    )
    terminate.add_argument(
        "-k",
        "--sigkill-timeout",
        type=int,
        default=DEFAULT_TIMEOUT_AFTER_SIGKILL,
        help="timeout to wait for an application to stop after SIGKILL.",
    )
    terminate.set_defaults(func=terminate_action)

    kill = command_group.add_parser(
        "kill",
        help=(
            "kill a user application process with SIGKILL then delete its"
            " associated resources."
        ),
    )
    kill.add_argument(
        "app_name", type=str, help="name of the application to kill."
    )
    kill.add_argument(
        "-t",
        "--sigkill-timeout",
        type=int,
        default=DEFAULT_TIMEOUT_AFTER_SIGKILL,
        help="timeout to wait for an application to stop after SIGKILL.",
    )
    kill.set_defaults(func=kill_action)

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


def run_mbl_app_lifecycle_manager():
    """Application main algorithm."""
    args = parse_args()

    set_log_verbosity(args.verbose)

    log.info("Starting mbl-app-lifecycle-manager")
    log.debug("Command line arguments:{}".format(args))

    args.func(args)


def _main():
    """Run mbl-app-lifecycle-manager."""
    try:
        run_mbl_app_lifecycle_manager()
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
