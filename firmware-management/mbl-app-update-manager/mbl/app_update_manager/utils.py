# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Contains utilities."""

import logging

__version__ = "1.1.0"

logging.basicConfig(
    # filename="/var/log/mbl-app-update-manager.log",
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
log = logging.getLogger(__name__)
