#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Simple command line interface for open ports checker."""

import argparse
import logging
import os
import mbl.open_ports_checker.open_ports_checker as opc


class GetValidFile(argparse.Action):
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
        description="Open ports checker",
    )

    parser.add_argument(
        "-w",
        "--white-list-file",
        metavar="FILE",
        action=GetValidFile,
        default=os.path.join(
            os.path.dirname(__file__), "white_list.json"
        ),
        help="Specify ports white list, input is .json file path",
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
    args = parser.parse_args()
    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logger = logging.getLogger("OpenPortsChecker")
    logger.debug("Command line arguments:{}".format(args))

    open_ports_checker = opc.OpenPortsChecker(args.white_list_file)
    ret = open_ports_checker.run_check()
    if ret == opc.Status.SUCCESS:
        logger.info("Operation successful")
    else:
        logger.error("Found black listed connections: {}".format(ret))
    return ret.value
