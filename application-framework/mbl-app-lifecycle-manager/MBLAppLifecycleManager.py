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
        result = self._run_command(command, working_dir)
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
        result = self._run_command(command)
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

    def stop_container(self, container_id, timeout):
        """
        Stop container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :param timeout: Timeout (seconds) to wait until container is stopped.
        in case timeout reached - a kill operation will be called.
        :return: AppLifecycleManagerErrors.SUCCESS
                 AppLifecycleManagerErrors.ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
                 AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        """
        logging.info("Stop container id: {}".format(container_id))
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state == AppLifecycleManagerContainerState.DOES_NOT_EXIST:
            logging.error(
                "Container id: {} does not exist.".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
        if state != AppLifecycleManagerContainerState.STOPPED:
            command = ["runc", "kill", container_id, "SIGTERM"]
            result = self._run_command(command)
            if result != AppLifecycleManagerErrors.SUCCESS:
                return result
            # Verify container has stopped using timeout
            start = time.monotonic()
            endtime = start + timeout
            while endtime > time.monotonic():
                state = self.get_container_state(container_id)
                if state == AppLifecycleManagerContainerState.UNKNOWN:
                    return (
                        AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
                    )
                if state == AppLifecycleManagerContainerState.STOPPED:
                    break
                time.sleep(SLEEP_INTERVAL)
        # If managed to stop container - delete it, else - kill it...
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.STOPPED:
            result = self._delete_container(container_id)
            if result != AppLifecycleManagerErrors.SUCCESS:
                return result
        else:
            logging.warning(
                "Stop Container id: {} failed. Trying to kill it".format(
                    container_id
                )
            )
            self.kill_container(container_id)
        # Make sure container does not exist
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state != AppLifecycleManagerContainerState.DOES_NOT_EXIST:
            logging.error(
                logging.error(
                    "Stop Container id: {} failed".format(container_id)
                )
            )
            return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        logging.info("Stop Container id: {} succeeded".format(container_id))
        return AppLifecycleManagerErrors.SUCCESS

    def kill_container(self, container_id):
        """
        Kill container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: AppLifecycleManagerErrors.SUCCESS
                 AppLifecycleManagerErrors.ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
                 AppLifecycleManagerErrors.ERR_CONTAINER_NOT_STOPPED
                 AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        """
        logging.info("Kill container id: {}".format(container_id))
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            logging.error(
                "Failed to get container id: {} status".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state == AppLifecycleManagerContainerState.DOES_NOT_EXIST:
            logging.error(
                "Container id: {} does not exist.".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_DOES_NOT_EXIST
        command = ["runc", "kill", container_id, "SIGKILL"]
        result = self._run_command(command)
        if result != AppLifecycleManagerErrors.SUCCESS:
            return result
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            logging.error(
                "Failed to get container id: {} status".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state != AppLifecycleManagerContainerState.STOPPED:
            logging.error(
                "Container id: {} did not stop.".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_NOT_STOPPED
        result = self._delete_container(container_id)
        if result != AppLifecycleManagerErrors.SUCCESS:
            return result
        # Make sure container does not exist
        state = self.get_container_state(container_id)
        if state == AppLifecycleManagerContainerState.UNKNOWN:
            logging.error(
                "Failed to get container id: {} status".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_CONTAINER_STATUS_UNKNOWN
        if state != AppLifecycleManagerContainerState.DOES_NOT_EXIST:
            logging.error("Kill Container id: {} failed".format(container_id))
            return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        logging.info("Kill Container id: {} succeeded".format(container_id))
        return AppLifecycleManagerErrors.SUCCESS

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
        command = ["runc", "state", container_id]
        logging.info("Executing command: {}".format(command))
        try:
            output = subprocess.check_output(
                command, stderr=subprocess.STDOUT
            ).decode("utf-8")
        except subprocess.CalledProcessError as e:
            output_str = e.output.decode()
            if output_str.find("does not exist"):
                logging.debug(
                    "Container {} does not exist.".format(container_id)
                )
                return AppLifecycleManagerContainerState.DOES_NOT_EXIST
            logging.error(
                "Get container {} status failed".format(container_id)
            )
            logging.error(
                "Error code: {}, output: {}".format(e.returncode, e.output)
            )
            return AppLifecycleManagerContainerState.UNKNOWN
        try:
            state_data = json.loads(output)
            status = state_data["status"]
        except ValueError as error:
            logging.error("JSON value error for container id {}: {}".format(
                container_id, error)
            )
            return AppLifecycleManagerContainerState.UNKNOWN
        except:
            logging.error("JSON parsing error for container id {}: {}".format(
                container_id)
            )
            return AppLifecycleManagerContainerState.UNKNOWN

        logging.debug("Container status: {}".format(status))
        if status == "created":
            return AppLifecycleManagerContainerState.CREATED
        if status == "running":
            return AppLifecycleManagerContainerState.RUNNING
        if status == "stopped":
            return AppLifecycleManagerContainerState.STOPPED

    def _delete_container(self, container_id):
        logging.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        result = self._run_command(command)
        if result != AppLifecycleManagerErrors.SUCCESS:
            logging.error(
                "Delete Container id: {} failed".format(container_id)
            )
            return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        return AppLifecycleManagerErrors.SUCCESS

    def _run_command(self, command, working_dir=None):
        logging.info("Executing command: {}".format(command))
        result = subprocess.run(command, cwd=working_dir)
        if result.returncode != 0:
            logging.error(
                "Error executing command: {}, failed with error: {}.".format(
                    command, result.returncode
                )
            )
            logging.error(
                "Error output: {}".format(result.stderr)
            )
            return AppLifecycleManagerErrors.ERR_OPERATION_FAILED
        return AppLifecycleManagerErrors.SUCCESS


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
        "--timeout",
        type=int,
        help="Maximum time (seconds) to wait for application container to exit"
        " during stop command",
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
            if args.timeout is None:
                logging.info("Missing timeout argument")
                return AppLifecycleManagerErrors.ERR_INVALID_ARGS
            return app_manager_lifecycle_mng.stop_container(
                args.stop_container, args.timeout
            )
        elif args.kill_container:
            return app_manager_lifecycle_mng.kill_container(
                args.kill_container
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
