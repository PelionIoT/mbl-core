#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


import argparse
import logging
import sys
import os
import mbl.cloud_connect_sample_application.cloud_connect_sample_application as ccapp

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
        description="Cloud Connect sample application",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="Increase output verbosity",
        action="store_true",
    )

    parser.add_argument(
        "-j",
        "--app-resource-definition-json-file",
        help="Path to the application resource definition json data",
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

    app = ccapp.CloudConnectSampleApp()

    logger.info("Call Cloud Connect sample application setup")
    app.setup()

    with open(args.app_resource_definition_json_file, 'r') as json_file:
        json_data = json_file.read().strip()
    
    logger.info("Call Cloud Connect sample application register_resources")
    app.register_resources(json_data)    
    
    logger.info("Cloud Connect sample application successfully finished")
    return 0

