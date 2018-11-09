#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""Simple command line interface for mbl firmware update manager."""

import sys
import argparse
import os
import subprocess
import logging
import tarfile
from enum import Enum
import mbl.firmware_update_manager.firmware_update_manager as fum


__version__ = "1.0.0"


class StoreValidFile(argparse.Action):
    """Utility class used in CLI argument parser scripts."""

    def __call__(self, parser, namespace, values, option_string=None):
        """Perform file validity checks."""
        prospective_file = values
        if not os.path.isfile(prospective_file):
            raise argparse.ArgumentTypeError(
                "file: {} not found".format(prospective_file)
            )
        if not os.access(prospective_file, os.R_OK):
            raise argparse.ArgumentTypeError(
                "file: {} is not a readable file".format(prospective_file)
            )
        setattr(namespace, self.dest, os.path.abspath(prospective_file))


def get_argument_parser():
    """
    Return argument parser.

    :return: parser
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Firmware update manager",
    )

    parser.add_argument(
        "-i",
        "--install-firmware",
        metavar="UPDATE_FIRMWARE_TAR_FILE",
        required=True,
        action=StoreValidFile,
        help="Install firmware from a firmware update tar file and reboot",
    )

    parser.add_argument(
        "-s",
        "--skip-reboot",
        help="Skip reboot after firmware update",
        action="store_true",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
    )

    return parser


def _main():
    parser = get_argument_parser()
    try:
        args = parser.parse_args()
    except SystemExit:
        return fum.Error.ERR_INVALID_ARGS.value

    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logger = logging.getLogger("mbl-firmware-update-manager")
    logger.info("Starting ARM FIRMWARE UPDATE MANAGER: {}".format(__version__))
    logger.info("Command line arguments:{}".format(args))

    ret = fum.Error.ERR_OPERATION_FAILED
    try:
        firmware_update_manager = fum.FirmwareUpdateManager()
        ret = firmware_update_manager.update_firmware(
            args.install_firmware, args.skip_reboot
        )
    except subprocess.CalledProcessError as e:
        logger.exception(
            "Operation failed with subprocess error code: {}".format(
                e.returncode
            )
        )
        return fum.Error.ERR_OPERATION_FAILED
    except OSError:
        logger.exception("Operation failed with OSError")
        return fum.Error.ERR_OPERATION_FAILED
    except Exception:
        logger.exception("Operation failed exception")
        return fum.Error.ERR_OPERATION_FAILED
    if ret == fum.Error.SUCCESS:
        logger.info("Operation successful")
    else:
        logger.error("Operation failed: {}".format(ret))
    return ret
