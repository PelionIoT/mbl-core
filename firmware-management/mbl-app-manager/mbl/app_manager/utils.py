# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
"""Contains utilities."""

import logging
import subprocess

__version__ = "1.1.0"

logging.basicConfig(
    # filename="/var/log/mbl-app-manager.log",
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
log = logging.getLogger(__name__)


def run_process(command, env_var, log_cmd=True):
    """Spawn a process and run a command.

    Control whether or not the command run in the process is logged.
    """
    if log_cmd:
        log.debug("Executing opkg command: {}".format(" ".join(command)))
    subprocess.check_call(command, env=env_var)


def get_output_from_process(command, env_var, format):
    """Run a process and return its output in a specified format."""
    return subprocess.check_output(command, env=env_var).decode(format)
