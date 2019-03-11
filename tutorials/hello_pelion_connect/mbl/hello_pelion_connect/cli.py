#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


import argparse
import logging
import sys
import os
import mbl.hello_pelion_connect.hello_pelion_connect as ccapp

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
        description="Hello Pelion Connect application",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
    )

    parser.add_argument(
        "-j",
        "--app-resource-definition-file",
        help="Path to the application resource definition data",
        metavar="FILE",
        action=GetValidFile,
        default=os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "app_resource_definition.json"
        ),
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
    logger = logging.getLogger("mbl-app-cloud-connect")
    logger.debug("Command line arguments:{}".format(args))

    app = ccapp.HelloPelionConnect()

    logger.info("Call Hello Pelion Connect application setup")
    app.setup()

    logger.info("Call Hello Pelion Connect application register_resources")
    app.register_resources(args.app_resource_definition_file)    
    
    logger.info("Hello Pelion Connect application successfully finished")
    return 0

