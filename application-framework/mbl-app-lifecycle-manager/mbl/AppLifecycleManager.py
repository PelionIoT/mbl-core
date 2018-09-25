#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""This script manages application lifecycles."""

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
    ERR_CONTAINER_NOT_STOPPED = 5
    ERR_CONTAINER_STATUS_UNKNOWN = 6
    ERR_INVALID_ARGS = 7
    ERR_CONTAINER_STOPPED = 8
    ERR_TIMEOUT = 9
    ERR_APP_NOT_FOUND = 10
    ERR_CONTAINER_RUNNING = 11
    ERR_DELETE_FAILED = 12
    ERR_CREATE_FAILED = 13
    ERR_START_FAILED = 14
    ERR_SIGNAL_FAILED = 15


class ContainerState(Enum):
    """AppLifecycleManager container states."""

    CREATED = 0
    RUNNING = 1
    STOPPED = 2
    DOES_NOT_EXIST = 3
    UNKNOWN = 4


class AppLifecycleManager:
    """Manage application lifecycle including run/stop/kill containers."""

    def __init__(self):
        """Create AppLifecycleManager object."""
        logging.info(
            "Creating AppLifecycleManager version {}".format(__version__)
        )

    def run_container(self, container_id, application_id):
        """
        Run container.

        In case container already exist an error code will return.
        :param container_id: Container ID
        :param application_id: Application ID (Application install directory
        under /home/app/)
        :return: Error.SUCCESS
                 Error.ERR_CREATE_FAILED
                 Error.ERR_START_FAILED
                 Error.ERR_CONTAINER_EXISTS
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_APP_NOT_FOUND
        """
        logging.info("Run container ID: {}".format(container_id))

        ret = self._create_container(container_id, application_id)
        if ret == Error.SUCCESS:
            ret = self._start_container(container_id)
        self._log_error_return("run_container", ret, container_id)
        return ret

    def stop_container(
        self,
        container_id,
        sigterm_timeout=DEFAULT_SIGTERM_TIMEOUT,
        sigkill_timeout=DEFAULT_SIGKILL_TIMEOUT,
    ):
        """
        Stop container and delete container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :param sigterm_timeout: Timeout (seconds) after sending a SIGTERM to a
                                container to wait until the container stops.
                                If the timeout is reached a SIGKILL will be
                                sent.
        :param sigkill_timeout: Timeout (seconds) after sending a SIGKILL to a
                                container to wait until the container stops.
                                If the timeout is reached ERR_TIMEOUT is
                                returned.
        :return: Error.SUCCESS
                 Error.ERR_SIGNAL_FAILED
                 Error.ERR_DELETE_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        logging.info("Stop container ID: {}".format(container_id))
        ret = self._stop_container_with_signal(
            container_id, "SIGTERM", sigterm_timeout
        )
        if ret == Error.ERR_TIMEOUT:
            logging.warning(
                "Container {} failed to stop within {}s of being sent a "
                "SIGTERM. Try sending a SIGKILL...".format(
                    container_id, sigterm_timeout
                )
            )
            ret = self._stop_container_with_signal(
                container_id, "SIGKILL", sigkill_timeout
            )
        if ret == Error.SUCCESS:
            ret = self._delete_container(container_id)
        self._log_error_return("stop_container", ret, container_id)
        return ret

    def kill_container(
        self, container_id, sigkill_timeout=DEFAULT_SIGKILL_TIMEOUT
    ):
        """
        Kill container and delete container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: Error.SUCCESS
                 Error.ERR_SIGNAL_FAILED
                 Error.ERR_DELETE_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        logging.info("Kill container ID: {}".format(container_id))
        ret = self._stop_container_with_signal(
            container_id, "SIGKILL", sigkill_timeout
        )
        if ret == Error.SUCCESS:
            ret = self._delete_container(container_id)
        self._log_error_return("kill_container", ret, container_id)
        return ret

    def get_container_state(self, container_id):
        """
        Return container state.

        :param container_id:
        :return: container state enum
                 ContainerState.CREATED
                 ContainerState.RUNNING
                 ContainerState.STOPPED
                 ContainerState.DOES_NOT_EXIST
                 ContainerState.UNKNOWN
        """
        # If container exists (e.g. created, started, stopped), runc will
        # return a string for initializing a dictionary with container state
        # values. e.g.
        # {
        #     "ociVersion": "<version>",
        #     "id": "<container id>",
        #     "pid": <number>,
        #     "status": "<created/started/stopped>",
        #     "bundle": "<container full path>",
        #     "rootfs": "<container rootfs path>",
        #     "created": "<creation date>",
        #     "owner": "<owner>"
        # }
        #
        # If container does not exist - runc will return string:
        # "container <container ID> does not exist"

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
                '"status" field not found in JSON output of "runc state" '
                "for container ID {}. Output was [{}]".format(
                    container_id, output
                )
            )
            return ContainerState.UNKNOWN
        status = state_data["status"]

        logging.debug("Container status: {}".format(status))
        if status == "created":
            return ContainerState.CREATED
        if status == "running":
            return ContainerState.RUNNING
        if status == "stopped":
            return ContainerState.STOPPED
        logging.error(
            'Unrecognized "status" value from "runc state" for container '
            "ID {}. Output was [{}]".format(container_id, output)
        )
        return ContainerState.UNKNOWN

    def _log_error_return(
        self, function_name, error, container_id, extra_info=""
    ):
        if error != Error.SUCCESS:
            if extra_info:
                extra_info = " ({})".format(extra_info)
            logging.warning(
                "{} returning {} for container {}{}".format(
                    function_name, error, container_id, extra_info
                )
            )

    def _create_container(self, container_id, application_id):
        """
        Create a container (but don't run it).

        Returns: Error.SUCCESS
                 Error.ERR_APP_NOT_FOUND
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_CONTAINER_EXISTS
                 Error.ERR_CREATE_FAILED
        """
        # Normally we could just run a command and if it fails, determine why
        # based on the return code and output of the command. In this case we
        # can't capture the output of "runc create" because runc will let the
        # container it creates inherit the stdio for runc, so attempting to
        # capture runc's output will also attempt to capture the containers
        # output.
        #
        # Check that the container does not exist before we try to create it so
        # that we can report an already existing container as an error. There
        # is a race condition here, but it is benign.

        state = self.get_container_state(container_id)
        result = Error.SUCCESS
        if state == ContainerState.UNKNOWN:
            result = Error.ERR_CONTAINER_STATUS_UNKNOWN
        elif state != ContainerState.DOES_NOT_EXIST:
            result = Error.ERR_CONTAINER_EXISTS
        if result != Error.SUCCESS:
            self._log_error_return("_create_container", result, container_id)
            return result
        working_dir = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        if not os.path.isdir(working_dir):
            result = Error.ERR_APP_NOT_FOUND
            self._log_error_return(
                "create_container",
                result,
                container_id,
                "Application ID {}".format(application_id),
            )
            return result
        logging.info("Create container: {}".format(container_id))
        _, result = self._run_command(
            ["runc", "create", container_id],
            working_dir,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if result == Error.ERR_OPERATION_FAILED:
            result = Error.ERR_CREATE_FAILED
        self._log_error_return("_create_container", result, container_id)
        return result

    def _start_container(self, container_id):
        """
        Start a pre-created container.

        Returns: Error.SUCCESS
                 Error.ERR_CONTAINER_STOPPED
                 Error.ERR_CONTAINER_RUNNING
                 Error.ERR_START_FAILED
        """
        logging.info("Start container: {}".format(container_id))
        output, result = self._run_command(["runc", "start", container_id])
        if result != Error.SUCCESS:
            if "cannot start a container that has stopped" in output:
                result = Error.ERR_CONTAINER_STOPPED
            elif "cannot start an already running container" in output:
                result = Error.ERR_CONTAINER_RUNNING
            else:
                result = Error.ERR_START_FAILED
        self._log_error_return("_start_container", result, container_id)
        return result

    def _signal_container(self, container_id, signal):
        """
        Send a signal to a container.

        Returns: Error.SUCCESS
                 Error.ERR_SIGNAL_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STOPPED
        """
        logging.info("Sending {} to container {}".format(signal, container_id))
        output, ret = self._run_command(["runc", "kill", container_id, signal])
        if ret == Error.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                ret = Error.ERR_CONTAINER_DOES_NOT_EXIST
            elif "process already finished" in output:
                ret = Error.ERR_CONTAINER_STOPPED
            else:
                ret = Error.ERR_SIGNAL_FAILED
        self._log_error_return("_signal_container", ret, container_id)
        return ret

    def _wait_for_container_state(self, container_id, required_state, timeout):
        """
        Wait until a container has a given state.

        Returns: Error.SUCCESS
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        start = time.monotonic()
        endtime = start + timeout
        while endtime > time.monotonic():
            state = self.get_container_state(container_id)
            if state == ContainerState.UNKNOWN:
                return Error.ERR_CONTAINER_STATUS_UNKNOWN
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
        """
        Send a container a signal and wait until it stops.

        The container already being stopped is not treated as an error.

        Returns: Error.SUCCESS
                 Error.ERR_SIGNAL_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_STATUS_UNKNOWN
                 Error.ERR_TIMEOUT
        """
        ret = self._signal_container(container_id, signal)
        if ret == Error.ERR_CONTAINER_STOPPED:
            ret = Error.SUCCESS
        if ret == Error.SUCCESS:
            ret = self._wait_for_container_state(
                container_id, ContainerState.STOPPED, timeout
            )
        self._log_error_return(
            "_stop_container_with_signal", ret, container_id
        )
        return ret

    def _delete_container(self, container_id):
        """
        Delete a container.

        Returns: Error.SUCCESS
                 Error.ERR_DELETE_FAILED
                 Error.ERR_CONTAINER_DOES_NOT_EXIST
                 Error.ERR_CONTAINER_RUNNING
        """
        logging.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        _, result = self._run_command(command)
        if result != Error.SUCCESS:
            if "does not exist" in output:
                result = Error.ERR_CONTAINER_DOES_NOT_EXIST
            elif "is not stopped: running" in output:
                result = Error.ERR_CONTAINER_RUNNING
            else:
                result = Error.ERR_DELETE_FAILED
        self._log_error_return("_delete_container", result, container_id)
        return result

    def _run_command(
        self,
        command,
        working_dir=None,
        stdin=None,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    ):
        """
        Run a command using subprocess.run(), with extra logging.

        Returns: 2-tuple (output, error) where output is the decoded (as UTF-8)
        stdout of the command if it is captured, and err is one of:
            Error.SUCCESS
            Error.OPERATION_FAILED
        """
        logging.debug("Executing command: {}".format(command))
        result = subprocess.run(
            command, cwd=working_dir, stdin=stdin, stdout=stdout, stderr=stderr
        )
        output = ""
        if result.stdout:
            output = result.stdout.decode("utf-8")
        if result.returncode != 0:
            logging.warning(
                "Command {} failed with status {} and output [{}]".format(
                    command, result.returncode, output
                )
            )
            return output, Error.ERR_OPERATION_FAILED
        return output, Error.SUCCESS
