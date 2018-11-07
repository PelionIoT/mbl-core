#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Simple command line interface for mbl app update manager."""

import argparse
import logging
import sys
import os
import mbl.app_update_manager.app_update_manager as aupm

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
        description="App update manager",
    )

    parser.add_argument(
        "-i",
        "--install-packages",
        metavar="UPDATE_PAYLOAD_TAR_FILE",
        required=True,
        action=StoreValidFile,
        help="Install packages from a firmware update tar file and run them",
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
        return aupm.Error.ERR_INVALID_ARGS.value

    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logger = logging.getLogger("mbl-app-update-manager")
    logger.info("Starting ARM APP UPDATE MANAGER: {}".format(__version__))
    logger.info("Command line arguments:{}".format(args))

    ret = aupm.Error.ERR_OPERATION_FAILED
    try:
        ret = aupm.install_and_run_apps_from_tar(args.install_packages)
    except subprocess.CalledProcessError as e:
        logger.exception(
            "Operation failed with subprocess error code: {}".format(
                e.returncode
            )
        )
        return aupm.Error.ERR_OPERATION_FAILED
    except OSError:
        logger.exception("Operation failed with OSError")
        return aupm.Error.ERR_OPERATION_FAILED
    except Exception:
        logger.exception("Operation failed exception")
        return aupm.Error.ERR_OPERATION_FAILED
    if ret == aupm.Error.SUCCESS:
        logger.info("Operation successful")
    else:
        logger.error("Operation failed: {}".format(ret))
    return ret


if __name__ == "__main__":
    _main()
