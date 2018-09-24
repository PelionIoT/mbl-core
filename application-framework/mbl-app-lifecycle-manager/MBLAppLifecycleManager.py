#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""This script manage application lifecycle."""

import sys
import argparse
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


class AppLifecycleManagerErrors(Enum):
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


class AppLifecycleManagerContainerState(Enum):
    """AppLifecycleManager container state."""

    CREATED = 0
    RUNNING = 1
    STOPPED = 2
    DOES_NOT_EXIST = 3
    UNKNOWN = 4


class AppLifecycleManager:
    """Manage application lifecycle including run/stop/kill containers."""

    def run_container(self, container_id, application_id):
        """
        Run container.

        In case container already exist an error code will return.
        :param container_id: Container ID
        :param application_id: Application ID (Application install directory
        under /home/app/)
        :return: AppLifecycleManagerErrors.SUCCESS
                 AppLifecycleManagerErrors.ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.ERR_CONTAINER_EXISTS
                 AppLifecycleManagerErrors.ERR_CONTAINER_NOT_CREATED
                 AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        """
        # Make sure container does not exist
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state != AppLifecycleManagerContainerState.DOES_NOT_EXIST:
            logging.error(
                "Container id: {} already exists.".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_EXISTS
        # Create container
        logging.info("Run container id: {}".format(container_id))
        working_dir = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        command = ["runc", "create", container_id]
        # We have to send stdio to /dev/null because otherwise the container will inherit our stdio
        _, result = self._run_command(command, working_dir, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if result != AppLifecycleManagerErrors.SUCCESS:
            return result
        # Make sure container created
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state != AppLifecycleManagerContainerState.CREATED:
            logging.error(
                "Container id: {} is not created.".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_NOT_CREATED
        # Start container
        command = ["runc", "start", container_id]
        _, result = self._run_command(command)
        if result != AppLifecycleManagerErrors.SUCCESS:
            return result
        # Make sure container started
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state == AppLifecycleManagerContainerState.RUNNING:
            logging.info("Run Container id: {} succeeded".format(container_id))
            return AppLifecycleManagerErrors.SUCCESS
        logging.error("Run Container id: {} failed".format(container_id))
        return AppLifecycleManagerErrors.ERR_OPERATION_FAILED

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
        :return: AppLifecycleManagerErrors.SUCCESS
                 AppLifecycleManagerErrors.ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
                 AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
                 AppLifecycleManagerErrors.ERR_TIMEOUT
        """
        logging.info("Stop container id: {}".format(container_id))
        ret = self._stop_container_with_signal(container_id, "SIGTERM", sigterm_timeout)
        if ret == AppLifecycleManagerErrors.ERR_TIMEOUT:
            logging.warning(
                "Stop Container id: {} failed. Trying to kill it".format(
                    container_id
                )
            )
            ret = self._stop_container_with_signal(container_id, "SIGKILL", sigkill_timeout)
        if ret != AppLifecycleManagerErrors.SUCCESS:
            return ret
        return self._delete_container(container_id)

    def kill_container(self, container_id, sigkill_timeout=DEFAULT_SIGKILL_TIMEOUT):
        """
        Kill container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: AppLifecycleManagerErrors.SUCCESS
                 AppLifecycleManagerErrors.ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
                 AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
                 AppLifecycleManagerErrors.ERR_TIMEOUT
        """
        logging.info("Kill container id: {}".format(container_id))
        ret = self._stop_container_with_signal(container_id, "SIGKILL", sigkill_timeout)
        if ret != AppLifecycleManagerErrors.SUCCESS:
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
                 AppLifecycleManagerContainerState.CREATED
                 AppLifecycleManagerContainerState.RUNNING
                 AppLifecycleManagerContainerState.STOPPED
                 AppLifecycleManagerContainerState.DOES_NOT_EXISTS
                 AppLifecycleManagerContainerState.UNKNOWN
        """
        output, ret = self._run_command(["runc", "state", container_id])
        if ret == AppLifecycleManagerErrors.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                return AppLifecycleManagerContainerState.DOES_NOT_EXIST
            return AppLifecycleManagerContainerState.UNKNOWN
        try:
            state_data = json.loads(output)
        except (ValueError, TypeError) as error:
            logging.exception(
                "JSON decode error for container id {}: {}".format(
                    container_id, error
                )
            )
            return AppLifecycleManagerContainerState.UNKNOWN

        if "status" not in state_data:
            logging.error(
                "\"status\" field not found in JSON output of \"runc state\" for container ID {}. Output was [{}]".format(
                    container_id, output
                )
            )
        status = state_data["status"]

        logging.debug("Container status: {}".format(status))
        if status == "created":
            return AppLifecycleManagerContainerState.CREATED
        if status == "running":
            return AppLifecycleManagerContainerState.RUNNING
        if status == "stopped":
            return AppLifecycleManagerContainerState.STOPPED
        logging.error(
            "Unrecognized \"status\" value from \"runc state\" for container ID {}. Output was [{}]".format(
                container_id, output
            )
        )
        return AppLifecycleManagerContainerState.UNKNOWN

    def _signal_container(self, container_id, signal):
        logging.info("Sending {} to container {}".format(signal, container_id))
        output, ret = self._run_command(["runc", "kill", container_id, signal])
        if ret == AppLifecycleManagerErrors.ERR_OPERATION_FAILED:
            if "does not exist" in output:
                return AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
            if "process already finished" in output:
                return AppLifecycleManagerErrors.ERR_CONTAINER_STOPPED
        return ret

    def _wait_for_container_state(self, container_id, required_state, timeout):
        start = time.monotonic()
        endtime = start + timeout
        while endtime > time.monotonic():
            state = self.get_container_state(container_id)
            if state == AppLifecycleManagerContainerState.UNKNOWN:
                return (
                    AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
                )
            if state == required_state:
                return AppLifecycleManagerErrors.SUCCESS
            time.sleep(SLEEP_INTERVAL)
        state = self.get_container_state(container_id)
        if state == required_state:
            return AppLifecycleManagerErrors.SUCCESS
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        return AppLifecycleManagerErrors.ERR_TIMEOUT


    def _stop_container_with_signal(self, container_id, signal, timeout):
        ret = self._signal_container(container_id, signal)
        if ret == AppLifecycleManagerErrors.ERR_CONTAINER_STOPPED:
            return AppLifecycleManagerErrors.SUCCESS
        if ret != AppLifecycleManagerErrors.SUCCESS:
            return ret
        return self._wait_for_container_state(container_id, AppLifecycleManagerContainerState.STOPPED, timeout)


    def _delete_container(self, container_id):
        logging.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        _, result = self._run_command(command)
        if result != AppLifecycleManagerErrors.SUCCESS:
            logging.error(
                "Delete Container id: {} failed".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        return AppLifecycleManagerErrors.SUCCESS

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
            return output, AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        return output, AppLifecycleManagerErrors.SUCCESS


def get_argument_parser():
    """
    Return argument parser.

    :return: parser
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="App lifecycle manager",
    )

    group = parser.add_mutually_exclusive_group(required=True)

    group.add_argument(
        "-r",
        "--run-container",
        metavar="RUN",
        help="Run container, input is container ID",
    )

    group.add_argument(
        "-s",
        "--stop-container",
        metavar="STOP",
        help="Stop container, input is container ID",
    )

    group.add_argument(
        "-k",
        "--kill-container",
        metavar="KILL",
        help="Kill container, input is container ID",
    )

    parser.add_argument("-a", "--application-id", help="Application ID")

    parser.add_argument(
        "-t",
        "--sigterm-timeout",
        type=int,
        help="Maximum time (seconds) to wait for application container to exit"
        " after sending a SIGTERM. Default is {}".format(DEFAULT_SIGTERM_TIMEOUT),
    )

    parser.add_argument(
        "-j",
        "--sigkill-timeout",
        type=int,
        help="Maximum time (seconds) to wait for application container to exit"
        " after sending a SIGKILL. Default is {}".format(DEFAULT_SIGKILL_TIMEOUT),
    )

    parser.add_argument(
        "-v",
        "--verbose",
        help="increase output verbosity",
        action="store_true",
    )

    return parser


def _main():
    parser = get_argument_parser()
    args = parser.parse_args()
    info_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=info_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logging.info("Starting ARM APP LIFECYCLE MANAGER: {}".format(__version__))
    logging.info("Command line arguments:{}".format(args))

    try:
        app_manager_lifecycle_mng = AppLifecycleManager()
        if args.run_container:
            if args.application_id is None:
                logging.info("Missing application-id argument")
                return AppLifecycleManagerErrors.ERR_INVALID_ARGS
            return app_manager_lifecycle_mng.run_container(
                args.run_container, args.application_id
            )
        elif args.stop_container:
            sigterm_timeout = args.sigterm_timeout or DEFAULT_SIGTERM_TIMEOUT
            sigkill_timeout = args.sigkill_timeout or DEFAULT_SIGKILL_TIMEOUT
            return app_manager_lifecycle_mng.stop_container(
                args.stop_container, sigterm_timeout, sigkill_timeout
            )
        elif args.kill_container:
            sigkill_timeout = args.sigkill_timeout or DEFAULT_SIGKILL_TIMEOUT
            return app_manager_lifecycle_mng.kill_container(
                args.kill_container, sigkill_timeout
            )
    except subprocess.CalledProcessError as e:
        logging.exception(
            "Operation failed with subprocess error code: " + str(e.returncode)
        )
        return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
    except OSError:
        logging.exception("Operation failed with OSError")
        return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
    except Exception:
        logging.exception("Operation failed exception")
        return AppLifecycleManagerErrors.ERR_OPERATION_FAILED


if __name__ == "__main__":
    sys.exit(_main())
