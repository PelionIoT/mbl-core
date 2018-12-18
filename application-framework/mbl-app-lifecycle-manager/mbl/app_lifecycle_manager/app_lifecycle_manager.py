#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

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
APPS_LOG_DIR = "/var/log/app"
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
        self.logger = logging.getLogger("AppLifecycleManager")
        self.logger.info(
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
        self.logger.info("Run container ID: {}".format(container_id))

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
        self.logger.info("Stop container ID: {}".format(container_id))
        ret = self._stop_container_with_signal(
            container_id, "SIGTERM", sigterm_timeout
        )
        if ret == Error.ERR_TIMEOUT:
            self.logger.warning(
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
        self.logger.info("Kill container ID: {}".format(container_id))
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

        :param container_id: container ID
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

        # Errors of get state command does not necessary means it is an actual
        # error. If for example container does not exist because it was never
        # started - its not an error.
        # It is up to the caller of this function to decide if logging
        # is needed.
        output, ret = self._run_command(["runc", "state", container_id], False)
        if ret == Error.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                return ContainerState.DOES_NOT_EXIST
            return ContainerState.UNKNOWN
        try:
            state_data = json.loads(output)
        except (ValueError, TypeError) as error:
            self.logger.exception(
                "JSON decode error for container ID {}: {}".format(
                    container_id, error
                )
            )
            return ContainerState.UNKNOWN

        if "status" not in state_data:
            self.logger.error(
                '"status" field not found in JSON output of "runc state" '
                "for container ID {}. Output was [{}]".format(
                    container_id, output
                )
            )
            return ContainerState.UNKNOWN
        status = state_data["status"]

        self.logger.debug("Container status: {}".format(status))
        if status == "created":
            return ContainerState.CREATED
        if status == "running":
            return ContainerState.RUNNING
        if status == "stopped":
            return ContainerState.STOPPED
        self.logger.error(
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
            self.logger.warning(
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
        bundle_path = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        if not os.path.isdir(bundle_path):
            result = Error.ERR_APP_NOT_FOUND
            self._log_error_return(
                "create_container",
                result,
                container_id,
                "Application ID {}".format(application_id),
            )
            return result

        with ContainerLogFile(self.logger, container_id) as log_file:
            log_file = log_file or subprocess.DEVNULL

            self.logger.info("Create container: {}".format(container_id))
            _, result = self._run_command(
                ["runc", "create", "--bundle", bundle_path, container_id],
                stdin=subprocess.DEVNULL,
                stdout=log_file,
                stderr=log_file,
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
        self.logger.info("Start container: {}".format(container_id))
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
        self.logger.info(
            "Sending {} to container {}".format(signal, container_id)
        )
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
        self.logger.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        output, result = self._run_command(command)
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
        log_error=True,
        stdin=None,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    ):
        """
        Run a command using subprocess.run(), with extra logging.

        In case log_error parameter is True - errors will be logged, if False
        error will not be logged.

        Returns: 2-tuple (output, error) where output is the decoded (as UTF-8)
        stdout of the command if it is captured, and err is one of:
            Error.SUCCESS
            Error.OPERATION_FAILED
        """
        self.logger.debug("Executing command: {}".format(command))
        result = subprocess.run(
            command, stdin=stdin, stdout=stdout, stderr=stderr
        )
        output = ""
        if result.stdout:
            output = result.stdout.decode("utf-8")
        if result.returncode != 0:
            if log_error:
                self.logger.error(
                    "Command {} failed with status {} and output [{}]".format(
                        command, result.returncode, output
                    )
                )
            return output, Error.ERR_OPERATION_FAILED
        return output, Error.SUCCESS


class ContainerLogFile:
    """
    Class to encapsulate opening of container log files.

    The point of having this class rather than using plain "open" is that:
    * It doesn't throw exceptions on failure. If we can't open a container's
      log file, just log an error and carry on.
    * It creates the log directory if it doesn't already exist.

    ContainerLogFile is really a "context manager" class (to be used in "with"
    statements).
    """

    def __init__(self, logger, container_id):
        """Create object to be used in "with" statement."""
        self.logger = logger
        self.container_id = container_id
        self.log_path = os.path.join(
            APPS_LOG_DIR, "{}.log".format(container_id)
        )

    def __enter__(self):
        """
        Open the container log file.

        Returns:
            A file object if the log file could be opened;
            None if the log file could not be opened.
        """
        try:
            os.makedirs(APPS_LOG_DIR, exist_ok=True)
        except OSError as error:
            self.logger.exception(
                "Failed to create container log directory {}".format(
                    APPS_LOG_DIR, error
                )
            )
            self.log_file = None
            return self.log_file

        try:
            self.log_file = open(self.log_path, "a")
            return self.log_file
        except OSError as error:
            self.logger.exception(
                "Failed to open log file {} for container {}".format(
                    self.log_file, self.container_id, error
                )
            )
            self.log_file = None
            return self.log_file

    def __exit__(self, type, value, tb):
        """Close the container log file."""
        if self.log_file:
            self.log_file.close()
        return False
