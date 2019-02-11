# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Application lifecycle."""

import time

from mbl.app_lifecycle_manager import container
from .utils import log

DEFAULT_TIMEOUT_AFTER_SIGTERM = 3
DEFAULT_TIMEOUT_AFTER_SIGKILL = 1

WAIT_SLEEP_INTERVAL = 0.1


def run_app(app_name, app_path):
    """Run an application."""
    log.debug("Run application '{}' at '{}'".format(app_name, app_path))
    if container.create(app_name, app_path):
        if container.start(app_name):
            log.info(
                "Application '{}' started from '{}'".format(app_name, app_path)
            )
            return True


def terminate_app(
    app_name,
    sigterm_timeout=DEFAULT_TIMEOUT_AFTER_SIGTERM,
    sigkill_timeout=DEFAULT_TIMEOUT_AFTER_SIGKILL,
):
    """Terminate an application.

    Kill the container with SIGTERM before deleting its resources.
    If the container's process is already killed, proceed with deleting its
    resouces.
    If killing the container with SIGTERM fails, kill it with SIGKILL instead.

    Return `True` if an application resources have been successfully
    deleted.
    """
    log.debug("Terminate application '{}'".format(app_name))

    try:
        if container.kill(app_name, "SIGTERM"):
            if _wait_for_app_stop(app_name, sigterm_timeout):
                return _delete_stopped_app(app_name)
    except container.ContainerKillError as error:
        if container.ContainerState.ALREADY_STOPPED.value in str(error):
            log.debug("Application '{}' already stopped".format(app_name))
            return _delete_stopped_app(app_name)
        elif container.ContainerState.DOES_NOT_EXIST.value in str(error):
            raise error
        else:
            if kill_app(app_name, sigkill_timeout):
                return True


def kill_app(app_name, timeout=DEFAULT_TIMEOUT_AFTER_SIGKILL):
    """Kill an application.

    Kill the application process's with SIGKILL before deleting its resources.

    Return `True` iff an application resources have been successfully
    deleted.
    """
    log.debug("Kill application '{}'".format(app_name))
    try:
        if container.kill(app_name, "SIGKILL"):
            if _wait_for_app_stop(app_name, timeout):
                return _delete_stopped_app(app_name)
    except container.ContainerKillError as error:
        if container.ContainerState.ALREADY_STOPPED.value in str(error):
            return _delete_stopped_app(app_name)


def _wait_for_app_stop(app_name, timeout):
    """Wait for an application to be stopped.

    Poll an application state until the timeout expiration or if the state is
    'stopped'.
    Return `True` iff the application status is stopped
    """
    endtime = time.monotonic() + timeout
    while endtime > time.monotonic():
        if container.get_state(app_name) == container.ContainerState.STOPPED:
            return True
        time.sleep(WAIT_SLEEP_INTERVAL)

    # check for state again as sleep() time is not guaranteed to be accurate
    # and it is possible to oversleep and miss the fact that the app has
    # already been stopped.
    if container.get_state(app_name) == container.ContainerState.STOPPED:
        return True

    return False


def _delete_stopped_app(app_name):
    """Delete a stopped application."""
    if container.delete(app_name):
        log.info("Application '{}' deleted".format(app_name))
        return True
