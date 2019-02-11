# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Contains utilities."""

import logging

__version__ = "1.1.0"

log = logging.getLogger(__name__)

HIGH_VERBOSITY_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
LOW_VERBOSITY_FORMAT = "%(message)s"


def set_log_verbosity(increase_verbosity):
    """Set the verbosity of the log output."""
    log_level = logging.DEBUG if increase_verbosity else logging.INFO
    log_format = (
        HIGH_VERBOSITY_FORMAT if increase_verbosity else LOW_VERBOSITY_FORMAT
    )

    log.setLevel(log_level)
    logging.basicConfig(level=log_level, format=log_format)
