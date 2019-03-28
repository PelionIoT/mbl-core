# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Contains utilities."""

import logging
import re

log = logging.getLogger(__name__)

VERBOSITY_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"


def set_log_verbosity(increase_verbosity):
    """Set the verbosity of the log output."""
    log_level = logging.DEBUG if increase_verbosity else logging.INFO

    log.setLevel(log_level)
    logging.basicConfig(level=log_level, format=VERBOSITY_FORMAT)


def alphanum_sort(l):
    """Sort the given list in ascending order the way that humans expect.

    i.e For a list l containing `["5", "24", "10"]`, the function sorts it as
    `["5", "10", "24"]` instead of `["10", "24", "5"]` as would Python List
    sort() method.
    """
    # key to use for the sort
    def convert(text):
        return int(text) if text.isdigit() else text

    def alphanum_key(key):
        return [convert(c) for c in re.split("([0-9]+)", key)]

    l.sort(key=alphanum_key)
