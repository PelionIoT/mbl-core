#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Simple command line interface for mbl app connectivity."""

import argparse
import logging
import sys
import mbl.app_cloud_connect.app_cloud_connect as alc


def get_argument_parser():
    """
    Return argument parser.

    :return: parser
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Application that used by tests to simulate real \
            application behavior",
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
    logger = logging.getLogger("mbl-app-cloud-connect")
    logger.debug("Command line arguments:{}".format(args))

    app = alc.AppConnectivity()

    logger.info("Call application Run")
    # Start the application main loop. This is a blocking call starting
    # D-Bus main loop. In order to stop application main loop call app.Stop().
    app.Run()
