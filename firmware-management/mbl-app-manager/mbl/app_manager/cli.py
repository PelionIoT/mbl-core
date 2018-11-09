#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Simple command line interface for mbl app manager."""

import argparse
import logging
import sys
import os
import mbl.app_manager.app_manager as apm

__version__ = "1.0.2"


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
        description="App manager",
    )

    group = parser.add_mutually_exclusive_group(required=True)

    group.add_argument(
        "-i",
        "--install-package",
        metavar="FILE",
        action=StoreValidFile,
        help="Install package on device, input is .ipk file path",
    )

    group.add_argument(
        "-f",
        "--force-install-package",
        metavar="FILE",
        action=StoreValidFile,
        help="Force install package on device, input is .ipk file path",
    )

    group.add_argument(
        "-r",
        "--remove-package-name",
        metavar="NAME",
        help="Remove installed package from device, input is package name",
    )

    group.add_argument(
        "-l",
        "--list-installed-packages",
        action="store_true",
        help="List installed packages on device",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="increase output verbosity",
        action="store_true",
    )

    return parser


def _main():
    parser = get_argument_parser()
    args = parser.parse_args()
    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logger = logging.getLogger("mbl-app-manager")
    logger.debug("Command line arguments:{}".format(args))

    app_manager = apm.AppManager()

    try:
        if args.install_package is not None:
            app_manager.install_package(args.install_package)
        elif args.remove_package_name is not None:
            app_manager.remove_package(args.remove_package_name)
        elif args.force_install_package is not None:
            app_manager.force_install_package(args.force_install_package)
        elif args.list_installed_packages:
            app_manager.list_installed_packages()
    except subprocess.CalledProcessError as e:
        logger.exception(
            "Operation failed with return error code: {}".format(
                str(e.returncode)
            )
        )
        return 1
    except OSError:
        logger.exception("Operation failed with OSError")
        return 2
