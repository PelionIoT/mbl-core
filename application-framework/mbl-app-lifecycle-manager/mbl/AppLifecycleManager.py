#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""This script manage application lifecycle."""

import sys
import os
import subprocess
import logging
import json
import time
from enum import Enum

__version__ = "1.0"
APPS_INSTALL_ROOT_DIR = "/home/app"
DEFAULT_SIGTERM_TIMEOUT = 3
DEFAULT_SIGKILL_TIMEOUT = 1
SLEEP_INTERVAL = 0.1


class Error(Enum):
    """AppLifecycleManager error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1
    ERR_CONTAINER_DOES_NOT_EXIST = 2
    ERR_CONTAINER_EXISTS = 3
    ERR_CONTAINER_NOT_CREATED = 4
    ERR_CONTAINER_NOT_STOPPED = 5
    ERR_CONTAINER_STATUS_UNKNOWN = 6
    ERR_INVALID_ARGS = 7
    ERR_CONTAINER_STOPPED = 8
    ERR_TIMEOUT = 9


class ContainerState(Enum):
    """AppLifecycleManager container state."""

    CREATED = 0
    RUNNING = 1
    STOPPED = 2
    DOES_NOT_EXIST = 3
    UNKNOWN = 4


class AppLifecycleManager:
    """Manage application lifecycle including run/stop/kill containers."""

    def __init__(self):
        logging.info("Creating AppLifecycleManager version {}".format(__version__))

    def run_container(self, container_id, application_id):
        """
        Run container.

        In case container already exist an error code will return.
        :param container_id: Container ID
        :param application_id: Application ID (Application install directory
        under /home/app/)
        :return: Error.SUCCESS
                 Error.ERR_OPERATION_FAILED
                 Error.ERR_CONTAINER_EXISTS
                 Error.ERR_CONTAINER_NOT_CREATED
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
        """
        # Make sure container does not exist
        state = self.get_container_state(container_id)
        if state == ContainerState.UNKNOWN:
            return Error.ERR_CONTAINER_STATUS_UNKNOWN
        if state != ContainerState.DOES_NOT_EXIST:
            logging.error(
                "Container ID: {} already exists.".format(container_id)
            )
            return Error.ERR_CONTAINER_EXISTS
        # Create container
        logging.info("Run container ID: {}".format(container_id))
        working_dir = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        command = ["runc", "create", container_id]
        # We have to send stdio to /dev/null because otherwise the container will inherit our stdio
        _, result = self._run_command(command, working_dir, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if result != Error.SUCCESS:
            return result
        # Make sure container created
        state = self.get_container_state(container_id)
        if state == ContainerState.UNKNOWN:
            return Error.ERR_CONTAINER_STATUS_UNKNOWN
        if state != ContainerState.CREATED:
            logging.error(
                "Container ID: {} is not created.".format(container_id)
            )
            return Error.ERR_CONTAINER_NOT_CREATED
        # Start container
        command = ["runc", "start", container_id]
        _, result = self._run_command(command)
        if result != Error.SUCCESS:
            return result
        # Make sure container started
        state = self.get_container_state(container_id)
        if state == ContainerState.UNKNOWN:
            return Error.ERR_CONTAINER_STATUS_UNKNOWN
        if state == ContainerState.RUNNING:
            logging.info("Run Container ID: {} succeeded".format(container_id))
            return Error.SUCCESS
        logging.error("Run Container ID: {} failed".format(container_id))
        return Error.ERR_OPERATION_FAILED

    def stop_container(self, container_id, sigterm_timeout=DEFAULT_SIGTERM_TIMEOUT, sigkill_timeout=DEFAULT_SIGKILL_TIMEOUT):
        """
        Stop container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :param sigterm_timeout: Timeout (seconds) after sending a SIGTERM to a
                                container to wait until the container stops.
                                If the timeout is reached a SIGKILL will be sent.
        :param sigkill_timeout: Timeout (seconds) after sending a SIGKILL to a
                                container to wait until the container stops.
                                If the timeout is reached ERR_TIMEOUT is returned.
        :return: Error.SUCCESS
                 Error.ERR_OPERATION_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        logging.info("Stop container ID: {}".format(container_id))
        ret = self._stop_container_with_signal(container_id, "SIGTERM", sigterm_timeout)
        if ret == Error.ERR_TIMEOUT:
            logging.warning(
                "Stop Container ID: {} failed. Trying to kill it".format(
                    container_id
                )
            )
            ret = self._stop_container_with_signal(container_id, "SIGKILL", sigkill_timeout)
        if ret != Error.SUCCESS:
            return ret
        return self._delete_container(container_id)

    def kill_container(self, container_id, sigkill_timeout=DEFAULT_SIGKILL_TIMEOUT):
        """
        Kill container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: Error.SUCCESS
                 Error.ERR_OPERATION_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        logging.info("Kill container ID: {}".format(container_id))
        ret = self._stop_container_with_signal(container_id, "SIGKILL", sigkill_timeout)
        if ret != Error.SUCCESS:
            return ret
        return self._delete_container(container_id)

    def get_container_state(self, container_id):
        """
        Return container state.

        If container exists (e.g. created, started, stopped), runc will return
        a string for initializing a dictionary with container state values.
        e.g.
        {
            "ociVersion": "<version>",
            "id": "<container id>",
            "pid": <number>,
            "status": "<created/started/stopped>",
            "bundle": "<container full path>",
            "rootfs": "<container rootfs path>",
            "created": "<creation date>",
            "owner": "<owner>"
        }

        If container does not exist - runc will return s string:
        "container <container ID> does not exist"

        :param container_id:
        :return: container state enum
                 ContainerState.CREATED
                 ContainerState.RUNNING
                 ContainerState.STOPPED
                 ContainerState.DOES_NOT_EXISTS
                 ContainerState.UNKNOWN
        """
        output, ret = self._run_command(["runc", "state", container_id])
        if ret == Error.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                return ContainerState.DOES_NOT_EXIST
            return ContainerState.UNKNOWN
        try:
            state_data = json.loads(output)
        except (ValueError, TypeError) as error:
            logging.exception(
                "JSON decode error for container ID {}: {}".format(
                    container_id, error
                )
            )
            return ContainerState.UNKNOWN

        if "status" not in state_data:
            logging.error(
                "\"status\" field not found in JSON output of \"runc state\" for container ID {}. Output was [{}]".format(
                    container_id, output
                )
            )
        status = state_data["status"]

        logging.debug("Container status: {}".format(status))
        if status == "created":
            return ContainerState.CREATED
        if status == "running":
            return ContainerState.RUNNING
        if status == "stopped":
            return ContainerState.STOPPED
        logging.error(
            "Unrecognized \"status\" value from \"runc state\" for container ID {}. Output was [{}]".format(
                container_id, output
            )
        )
        return ContainerState.UNKNOWN

    def _signal_container(self, container_id, signal):
        logging.info("Sending {} to container {}".format(signal, container_id))
        output, ret = self._run_command(["runc", "kill", container_id, signal])
        if ret == Error.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                return Error.ERR_CONTAINER_DOES_NOT_EXIST
            if "process already finished" in output:
                return Error.ERR_CONTAINER_STOPPED
        return ret

    def _wait_for_container_state(self, container_id, required_state, timeout):
        start = time.monotonic()
        endtime = start + timeout
        while endtime > time.monotonic():
            state = self.get_container_state(container_id)
            if state == ContainerState.UNKNOWN:
                return (
                    Error.ERR_CONTAINER_STATUS_UNKNOWN
                )
            if state == required_state:
                return Error.SUCCESS
            time.sleep(SLEEP_INTERVAL)
        state = self.get_container_state(container_id)
        if state == required_state:
            return Error.SUCCESS
        if state == ContainerState.UNKNOWN:
            return Error.ERR_CONTAINER_STATUS_UNKNOWN
        return Error.ERR_TIMEOUT


    def _stop_container_with_signal(self, container_id, signal, timeout):
        ret = self._signal_container(container_id, signal)
        if ret == Error.ERR_CONTAINER_STOPPED:
            return Error.SUCCESS
        if ret != Error.SUCCESS:
            return ret
        return self._wait_for_container_state(container_id, ContainerState.STOPPED, timeout)


    def _delete_container(self, container_id):
        logging.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        _, result = self._run_command(command)
        if result != Error.SUCCESS:
            logging.error(
                "Delete Container ID: {} failed".format(container_id)
            )
            return Error.ERR_OPERATION_FAILED
        return Error.SUCCESS

    def _run_command(self, command, working_dir=None, stdin=None, stdout=subprocess.PIPE, stderr=subprocess.STDOUT):
        logging.debug("Executing command: {}".format(command))
        result = subprocess.run(
            command,
            cwd=working_dir,
            stdin=stdin,
            stdout=stdout,
            stderr=stderr,
        )
        output = ""
        if result.stdout:
            output = result.stdout.decode("utf-8")
        if result.returncode != 0:
            logging.error(
                "Command {} failed with status {} and output [{}]".format(
                    command, result.returncode, output
                )
            )
            return output, Error.ERR_OPERATION_FAILED
        return output, Error.SUCCESS
