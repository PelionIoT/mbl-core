# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Application meta data parser.

The application information is fetched from the ipk 'control' file.
The control file contains entries of the form 'field: value' where each
field-value pair only occupy one line ended with a newline character.
"""

import subprocess

from .utils import log


def parse_app_info(app_pkg):
    """Parse the app info from the ipk control data."""
    pkg_info = {}
    log.debug("Parse package info from {}".format(app_pkg))
    cmd = ["opkg", "info", app_pkg]
    app_ctrl_data = subprocess.check_output(cmd).decode("utf-8")
    log.debug("Package info:\n{}".format(app_ctrl_data))

    # create a list of all the fields in the application control data
    lines = list(app_ctrl_data.split("\n"))

    for line in lines:
        field_and_value = line.split(":")
        # check that a valid entry was found
        if len(field_and_value) != 1:
            field, value = field_and_value
            field = field.rstrip()
            value = value.lstrip()
            # create a dictionary that has the fields as keys
            # and the values as...well values.
            pkg_info[field] = value

    if "Package" not in pkg_info:
        msg = "'{}' control data does not have a 'Package' field".format(
            app_pkg
        )
        raise AppInfoParserError(msg)

    log.debug("Package '{}' info parsing completed".format(app_pkg))
    return pkg_info


class AppInfoParserError(Exception):
    """An exception for an incorrect app information."""
