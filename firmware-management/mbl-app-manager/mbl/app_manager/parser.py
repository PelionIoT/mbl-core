# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Application meta data parser."""

from .utils import log, get_output_from_process


class ApplicationInfoParser:
    """Parse the application information."""

    def __init__(self, opkg_env):
        """Create an application information parser."""
        self._opkg_env = opkg_env
        self.pkg_info = {}

    # ---------------------------- Public Methods -----------------------------

    def get_pkg_info(self, app_pkg):
        """
        Get a dictionary that hold the various fields of an ipk control data.

        Assume the 'Description' field is last and its value is followed by a
        double new line char.
        """
        cmd = ["opkg", "info", app_pkg]
        app_ctrl_data = get_output_from_process(
            command=cmd, env_var=self._opkg_env, format="utf-8"
        )
        log.info("Package info:\n {}".format(app_ctrl_data))

        # All package info fields includes "\n" only once, while description
        # might have several. Since we use the new line as separator in the
        # split operation, we want to get rid off the newline in this field.
        app_ctrl_data_desc_start = app_ctrl_data.find("Description: ")
        app_ctrl_data_desc_end = app_ctrl_data.find("\n\n")
        app_ctrl_data_desc = app_ctrl_data[
            app_ctrl_data_desc_start:app_ctrl_data_desc_end
        ]
        app_ctrl_data_desc_no_newline = app_ctrl_data_desc.replace("\n", "")
        app_ctrl_data = app_ctrl_data.replace(
            app_ctrl_data_desc, app_ctrl_data_desc_no_newline
        )

        # create a list of all the fields in the application control data
        app_ctrl_data_fields = list(app_ctrl_data.split("\n"))

        keys_and_values = []
        for field in app_ctrl_data_fields:
            if field:
                # create a list containing field key and its value
                key_and_value = field.split(":")
                key_and_value[1] = key_and_value[1].lstrip(" ")
                # collect all fields key-value pairs
                keys_and_values.append(key_and_value)

        # Fill dictionary to look something like:
        # [Package][Package name]
        # [Description][Package description...]
        # [Architecture][...]
        #  and so on.
        for key_and_value in keys_and_values:
            self.pkg_info[key_and_value[0]] = key_and_value[1]
