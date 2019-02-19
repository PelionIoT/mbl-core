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
    container.create(app_name, app_path)
    container.start(app_name)


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
    """
    log.debug("Terminate application '{}'".format(app_name))
    try:
        container.kill(app_name, "SIGTERM")
    except container.ContainerKillError as error:
        log.debug(
            "Failed to terminate '{}', error: '{}".format(app_name, str(error))
        )
        if any(
            x in str(error)
            for x in [
                container.ContainerState.ALREADY_STOPPED.value,
                container.ContainerState.NOT_RUNNING.value,
            ]
        ):
            log.debug("Application '{}' already stopped".format(app_name))
            _delete_stopped_app(app_name)
        elif container.ContainerState.DOES_NOT_EXIST.value in str(error):
            return
    else:
        try:
            _wait_for_app_stop(app_name, sigterm_timeout)
        except AppStopTimeoutError:
            log.debug(
                "Failed to stop '{}' in '{}' seconds. Try SIGKILL.".format(
                    app_name, sigterm_timeout
                )
            )
            kill_app(app_name, sigkill_timeout)
        else:
            _delete_stopped_app(app_name)


def kill_app(app_name, timeout=DEFAULT_TIMEOUT_AFTER_SIGKILL):
    """Kill an application.

    Kill the application process's with SIGKILL before deleting its resources.
    """
    log.debug("Kill application '{}'".format(app_name))
    try:
        container.kill(app_name, "SIGKILL")
    except container.ContainerKillError as error:
        log.debug(
            "Failed to kill '{}', error: '{}".format(app_name, str(error))
        )
        if any(
            x in str(error)
            for x in [
                container.ContainerState.ALREADY_STOPPED.value,
                container.ContainerState.NOT_RUNNING.value,
            ]
        ):
            _delete_stopped_app(app_name)
        else:
            raise error
    else:
        _wait_for_app_stop(app_name, timeout)
        _delete_stopped_app(app_name)


def _delete_stopped_app(app_name):
    """Delete a stopped application."""
    container.delete(app_name)
    log.info("Application '{}' deleted".format(app_name))


def _wait_for_app_stop(app_name, timeout):
    """Wait for an application to be stopped.

    Poll an application state until the timeout expiration or if the state is
    'stopped'.
    """
    log.debug("Wait {} seconds for '{}' to stop".format(timeout, app_name))
    endtime = time.monotonic() + timeout
    while endtime > time.monotonic():
        app_state = container.get_state(app_name)
        log.debug("'{}' state is '{}'".format(app_name, app_state))
        if app_state == container.ContainerState.STOPPED:
            return
        time.sleep(WAIT_SLEEP_INTERVAL)

    # check for state again as sleep() time is not guaranteed to be accurate
    # and it is possible to oversleep and miss the fact that the app has
    # already been stopped.
    app_state = container.get_state(app_name)
    log.debug("'{}' state is '{}'".format(app_name, app_state))
    if app_state == container.ContainerState.STOPPED:
        return

    raise AppStopTimeoutError(
        "'{}' was not stopped after '{}' seconds".format(app_name, timeout)
    )


class AppStopTimeoutError(Exception):
    """Error to indicate an application failure to stop within a given time."""
