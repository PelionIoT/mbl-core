# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Contains utilities."""

import logging
import re

import mbl.app_manager.utils as apu
import mbl.app_lifecycle_manager.utils as alu

log = logging.getLogger(__name__)

VERBOSITY_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"


def set_log_verbosity(increase_verbosity):
    """Set the verbosity of the log output."""
    log_level = logging.DEBUG if increase_verbosity else logging.INFO

    log.setLevel(log_level)
    logging.basicConfig(level=log_level, format=VERBOSITY_FORMAT)
    # Syncronize the log level of all MBL dependencies
    alu.set_log_verbosity(increase_verbosity)
    apu.set_log_verbosity(increase_verbosity)


def human_sort(l):
    """Sort the given list in the way that humans expect."""
    # key to use for the sort
    def convert(text):
        return int(text) if text.isdigit() else text

    def alphanum_key(key):
        return [convert(c) for c in re.split("([0-9]+)", key)]

    l.sort(key=alphanum_key)
