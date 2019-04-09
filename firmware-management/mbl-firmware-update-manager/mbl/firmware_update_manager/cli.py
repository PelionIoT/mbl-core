# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Simple command line interface for mbl firmware update manager."""

import argparse
import sys
from enum import Enum

from .manager import FirmwareUpdateManager
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
        help="full path of the update package containing firmware to install",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--assume-no",
        action="store_true",
        help=(
            "Automatic no to prompts. Assume 'no' as answer to all prompts"
            " and run non-interactively."
        ),
    )
    group.add_argument(
        "--assume-yes",
        action="store_true",
        help=(
            "Automatic yes to prompts. Assume 'yes' as answer to all prompts"
            " and run non-interactively."
        ),
    )

    parser.add_argument(
        "--keep",
        action="store_true",
        help=(
            "do not delete the update package or the"
            " header file from the device when done"
        ),
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

    handler = FirmwareUpdateManager(args.update_package)
    handler.install_header()
    handler.install_firmware(args.keep, args.assume_yes, args.assume_no)


def _main():
    """Run mbl-firmware-update-manager."""
    try:
        run_mbl_firmware_update_manager()
    except Exception as error:
        print(error, file=sys.stderr)
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
