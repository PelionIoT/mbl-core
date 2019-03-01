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

from mbl.app_manager.manager import APPS_PATH

from .container import get_oci_bundle_paths
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


def run_app_from_parent_dir(name):
    """Run an application from its parent directory.

    For an application `app_a` located at `/home/app/app_a`,
    run the application version with the lowest number and remove
    all other versions found if any.
    i.e If `/home/app/app_a/9` and `/home/app/app_a/10` are found,
    run `/home/app/app_a/9` and remove `/home/app/app_a/10` if
    successful.
    """
    app_parent_dir = os.path.join(APPS_PATH, name)

    if not os.path.isdir(app_parent_dir):
        raise Exception("'{}' is not installed".format(name))

    versions = get_oci_bundle_paths(app_parent_dir)

    if versions:
        human_sort(versions)
        valid_version = versions.pop(0)
        run_app(name, valid_version)

        # clean up other app versions if any
        if versions:
            for invalid_version in versions:
                shutil.rmtree(invalid_version)


def run_action(args):
    """Entry point for the 'run' cli command."""
    run_app_from_parent_dir(args.app_name)


def run_all_action(args):
    """Entry point for the `run-all` cli command."""
    # Get the immediate subdirectory names, these names are the names the
    # applications installed on the system.
    app_names = next(os.walk(APPS_PATH))[1]
    for app_name in app_names:
        run_app_from_parent_dir(app_name)


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

    run = command_group.add_parser(
        "run", help=("run a user application found at {}.".format(APPS_PATH))
    )
    run.add_argument(
        "app_name",
        type=str,
        help="name the application will be referred as"
        " once it has been started.",
    )
    run.set_defaults(func=run_action)

    run_all = command_group.add_parser(
        "run-all",
        help=("run all user applications found at {}.".format(APPS_PATH)),
    )
    run_all.set_defaults(func=run_all_action)

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
