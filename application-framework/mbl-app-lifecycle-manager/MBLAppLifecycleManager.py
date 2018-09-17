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
import ast
import time
from enum import Enum

__version__ = "1.0"
APPS_INSTALL_ROOT_DIR = "/home/app"
CONTAINER_STATUS_CREATED = "created"
CONTAINER_STATUS_RUNNING = "running"
CONTAINER_STATUS_STOPPED = "stopped"
CONTAINER_STATUS_NOT_EXIST = "does not exist"
SLEEP_INTERVAL = 0.01


class AppLifecycleManagerErrors(Enum):
    """AppLifecycleManager error codes."""

    STATUS_SUCCESS = 0
    STATUS_ERR_OPERATION_FAILED = 1
    STATUS_ERR_CONTAINER_DOES_NOT_EXIST = 2
    STATUS_ERR_CONTAINER_EXISTS = 3
    STATUS_ERR_CONTAINER_NOT_CREATED = 4
    STATUS_ERR_CONTAINER_NOT_STOPPED = 5


class AppLifecycleManager:
    """Manage application lifecycle including run/stop/kill containers."""

    def run_container(self, container_id, application_id):
        """
        Run container.

        In case container already exist an error code will return.
        :param container_id: Container ID
        :param application_id: Application ID (Application install directory
        under /home/app/)
        :return: AppLifecycleManagerErrors.STATUS_SUCCESS
                 AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_EXISTS
                 AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_NOT_CREATED
                 Any other error returned by subprocess.run()
        """
        # Make sure container does not exist
        container_exists = self.container_exists(container_id)
        if container_exists:
            logging.error(
                "Container id: {} already exists.".format(container_id)
            )
            return AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_EXISTS
        # Create container
        logging.info("Run container id: {}".format(container_id))
        working_dir = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        command = ["runc", "create", container_id]
        result = self._run_command(command, working_dir)
        if result != 0:
            return result
        # Make sure container created
        if not self.container_created(container_id):
            logging.error(
                "Container id: {} is not created.".format(container_id)
            )
            return AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_NOT_CREATED
        # Start container
        command = ["runc", "start", container_id]
        result = self._run_command(command)
        if result != 0:
            return result
        # Make sure container started
        if self.container_running(container_id):
            logging.info("Run Container id: {} succeeded".format(container_id))
            return AppLifecycleManagerErrors.STATUS_SUCCESS
        logging.error("Run Container id: {} failed".format(container_id))
        return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED

    def stop_container(self, container_id, timeout):
        """
        Stop container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :param timeout: Timeout (seconds) to wait until container is stopped.
        in case timeout reached - a kill operation will be called.
        :return: AppLifecycleManagerErrors.STATUS_SUCCESS
                 AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_DOES_NOT_EXIST
                 Any other error returned by subprocess.run()
        """
        logging.info("Stop container id: {}".format(container_id))
        container_exists = self.container_exists(container_id)
        if not container_exists:
            logging.error(
                "Container id: {} does not exist.".format(container_id)
            )
            return (
                AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_DOES_NOT_EXIST
            )
        container_stopped = self.container_stopped(container_id)
        if not container_stopped:
            command = ["runc", "kill", container_id, "SIGTERM"]
            result = self._run_command(command)
            if result != 0:
                return result
            # Verify container has stopped using timeout
            while timeout > 0:
                container_stopped = self.container_stopped(container_id)
                if container_stopped:
                    break
                time.sleep(SLEEP_INTERVAL)
                timeout = timeout - SLEEP_INTERVAL
        # If managed to stop container - delete it, else - kill it...
        if container_stopped:
            result = self._delete_container(container_id)
            if result != AppLifecycleManagerErrors.STATUS_SUCCESS:
                return result
        else:
            logging.warning(
                "Stop Container id: {} failed. Trying to kill it".format(
                    container_id
                )
            )
            self.kill_container(container_id)
        # Make sure container does not exist
        if self.container_exists(container_id):
            logging.error(
                logging.error(
                    "Stop Container id: {} failed".format(container_id)
                )
            )
            return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
        logging.info("Stop Container id: {} succeeded".format(container_id))
        return AppLifecycleManagerErrors.STATUS_SUCCESS

    def kill_container(self, container_id):
        """
        Kill container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: AppLifecycleManagerErrors.STATUS_SUCCESS
                 AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
                 AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_DOES_NOT_EXIST
                 AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_NOT_STOPPED
                 Any other error returned by subprocess.run()
        """
        logging.info("Kill container id: {}".format(container_id))
        container_exists = self.container_exists(container_id)
        if not container_exists:
            logging.error(
                "Container id: {} does not exist.".format(container_id)
            )
            return (
                AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_DOES_NOT_EXIST
            )
        command = ["runc", "kill", container_id, "SIGKILL"]
        result = self._run_command(command)
        if result != 0:
            return result
        container_stopped = self.container_stopped(container_id)
        if not container_stopped:
            logging.error(
                "Container id: {} did not stop.".format(container_id)
            )
            return AppLifecycleManagerErrors.STATUS_ERR_CONTAINER_NOT_STOPPED
        result = self._delete_container(container_id)
        if result != AppLifecycleManagerErrors.STATUS_SUCCESS:
            return result
        # Make sure container does not exist
        if self.container_exists(container_id):
            logging.error("Kill Container id: {} failed".format(container_id))
            return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
        logging.info("Kill Container id: {} succeeded".format(container_id))
        return AppLifecycleManagerErrors.STATUS_SUCCESS

    def container_exists(self, container_id):
        """
        Check is container exist (e.g. created/running, and so on.

        :param container_id: Container ID
        :return: True if exist, False if not exist
        """
        container_status = self._get_container_status(container_id)
        return container_status != CONTAINER_STATUS_NOT_EXIST

    def container_created(self, container_id):
        """
        Check if container is already created.

        :param container_id: Container ID
        :return: True is created, False if not created
        """
        container_status = self._get_container_status(container_id)
        return container_status == CONTAINER_STATUS_CREATED

    def container_running(self, container_id):
        """
        Check if container is running.

        :param container_id: Container ID
        :return: True if running, False if not running
        """
        container_status = self._get_container_status(container_id)
        return container_status == CONTAINER_STATUS_RUNNING

    def container_stopped(self, container_id):
        """
        Check if container is stopped.

        :param container_id: Container ID
        :return: True if stopped, False if not stopped
        """
        container_status = self._get_container_status(container_id)
        return container_status == CONTAINER_STATUS_STOPPED

    def _get_container_status(self, container_id):
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
        :return: container status string
        """
        command = ["runc", "state", container_id]
        logging.debug("Executing command: " + " ".join(command))
        try:
            output = subprocess.check_output(
                command, stderr=subprocess.STDOUT
            ).decode("utf-8")
        except subprocess.CalledProcessError as e:
            output_str = e.output.decode()
            if output_str.find(CONTAINER_STATUS_NOT_EXIST):
                logging.debug(
                    "Container {} does not exist.".format(container_id)
                )
                return CONTAINER_STATUS_NOT_EXIST
            return e.returncode
        state_dictionary = ast.literal_eval(output)
        status = state_dictionary["status"]
        logging.info("Container status: {}".format(status))
        return status

    def _delete_container(self, container_id):
        logging.info("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        result = self._run_command(command)
        if result != 0:
            logging.error(
                logging.error(
                    "Delete Container id: {} failed".format(container_id)
                )
            )
            return result
        return AppLifecycleManagerErrors.STATUS_SUCCESS

    def _run_command(self, command, working_dir=None):
        logging.info("Executing command: " + " ".join(command))
        result = subprocess.run(command, cwd=working_dir)
        if result.returncode != 0:
            logging.error(
                "Error executing command failed with error: {}.".format(
                    result.returncode
                )
            )
        return result.returncode


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
    info_level = logging.info if args.verbose else logging.INFO

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
                return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
            return app_manager_lifecycle_mng.run_container(
                args.run_container, args.application_id
            )
        elif args.stop_container:
            if args.timeout is None:
                logging.info("Missing timeout argument")
                return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
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
        return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
    except OSError:
        logging.exception("Operation failed with OSError")
        return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED
    except Exception:
        logging.exception("Operation failed exception")
        return AppLifecycleManagerErrors.STATUS_ERR_OPERATION_FAILED


if __name__ == "__main__":
    sys.exit(_main())
