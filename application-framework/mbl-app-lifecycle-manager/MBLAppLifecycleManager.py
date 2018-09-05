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

__version__ = "1.0"
APPS_INSTALL_ROOT_DIR = "/home/app"
CONTAINER_STATUS_CREATED = "created"
CONTAINER_STATUS_RUNNING = "running"
CONTAINER_STATUS_STOPPED = "stopped"
CONTAINER_STATUS_NOT_EXIST = "does not exist"


class AppLifecycleManager:
    """Manage application lifecycle including run/stop/kill containers."""

    def __init__(self):
        """Init AppLifecycleManager class."""

    def run_container(self, container_id, application_id, context):
        """
        Run container.

        In case container already exist an error code will return.
        :param container_id: Container ID
        :param application_id: Application ID (Application install directory
        under /home/app/
        :param context: Application specific context string such as a URI
        :return: 0 in case of success, 1 in case of failure
        """
        # Create container
        logging.debug("Start container: {}".format(container_id))
        # Make sure container does not exists
        is_exist = self.is_container_exist(container_id)
        if is_exist:
            logging.error("Container {} already exists.".format(container_id))
            return 1
        working_dir = os.path.join(APPS_INSTALL_ROOT_DIR, application_id)
        command = ["runc", "create", container_id]
        logging.debug("Executing command: " + " ".join(command))
        subprocess.run(command, cwd=working_dir)
        # Start container
        command = ["runc", "start", container_id]
        logging.debug("Executing command: " + " ".join(command))
        subprocess.run(command)
        # Make sure container started
        if self.is_container_running(container_id):
            logging.info("Container {} is running.".format(container_id))
            return 0
        return 1

    def stop_container(self, container_id, timeout):
        """
        Stop container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :param timeout: Timeout (seconds) to wait until container is stopped.
        in case timeout reached - a kill operation will be called.
        :return: 0 in case of success, 1 in case of failure
        """
        logging.debug("Stop container: {}".format(container_id))
        is_exist = self.is_container_exist(container_id)
        if not is_exist:
            logging.error("Container {} does not exists.".format(container_id))
            return 1
        stopped = self.is_container_stopped(container_id)
        if not stopped:
            command = ["runc", "kill", container_id, "SIGTERM"]
            logging.debug("Executing command: " + " ".join(command))
            subprocess.run(command)
            # Verify container has stopped using timeout
            while timeout > 0:
                stopped = self.is_container_stopped(container_id)
                if stopped:
                    break
                time.sleep(1)
                timeout = timeout - 1
        if stopped:
            self._delete_container(container_id)
        else:
            self.kill_container(container_id)
        # Make sure container does not exist
        if self.is_container_exist(container_id):
            logging.error("Container {} did not stop.".format(container_id))
            return 1
        logging.info("Container {} is stopped.".format(container_id))
        return 0

    def kill_container(self, container_id):
        """
        Kill container.

        In case container does not exist an error code will return.
        :param container_id: Container ID
        :return: 0 in case of success, 1 in case of failure
        """
        logging.debug("Kill container: {}".format(container_id))
        is_exist = self.is_container_exist(container_id)
        if not is_exist:
            logging.error("Container {} does not exists.".format(container_id))
            return 1
        command = ["runc", "kill", container_id, "SIGKILL"]
        logging.debug("Executing command: " + " ".join(command))
        subprocess.run(command)
        self._delete_container(container_id)
        # Make sure container does not exist
        if self.is_container_exist(container_id):
            logging.error("Container {} did not stop.".format(container_id))
            return 1
        logging.info("Container {} is killed.".format(container_id))
        return 0

    def is_container_exist(self, container_id):
        """
        Check is container exist (e.g. created/running, and so on.

        :param container_id: Container ID
        :return: True is exist, False if not exist
        """
        container_status = self._get_container_status(container_id)
        return container_status != CONTAINER_STATUS_NOT_EXIST

    def is_container_created(self, container_id):
        """
        Check if container is already created.

        :param container_id: Container ID
        :return: True is created, False if not created
        """
        container_status = self._get_container_status(container_id)
        return container_status == CONTAINER_STATUS_CREATED

    def is_container_running(self, container_id):
        """
        Check if container is running.

        :param container_id: Container ID
        :return: True is running, False if not running
        """
        container_status = self._get_container_status(container_id)
        return container_status == CONTAINER_STATUS_RUNNING

    def is_container_stopped(self, container_id):
        """
        Check if container is stopped.

        :param container_id: Container ID
        :return: True is stopped, False if not stopped
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
            output = subprocess.check_output(command).decode("utf-8")
        except subprocess.CalledProcessError:
            logging.debug("Container {} does not exist.".format(container_id))
            return CONTAINER_STATUS_NOT_EXIST
        state_dictionary = ast.literal_eval(output)
        status = state_dictionary["status"]
        logging.debug("Container status: {}".format(status))
        return status

    def _delete_container(self, container_id):
        logging.debug("Delete container: {}".format(container_id))
        command = ["runc", "delete", container_id]
        logging.debug("Executing command: " + " ".join(command))
        subprocess.run(command)


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
        "-c",
        "--context",
        help="Application specific context string such as a URI",
    )

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
    debug_level = logging.DEBUG if args.verbose else logging.INFO

    logging.basicConfig(
        level=debug_level,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    logging.debug("Starting ARM APP LIFECYCLE MANAGER: {}".format(__version__))
    logging.info("Command line arguments:{}".format(args))

    try:
        app_manager_lifecycle_mng = AppLifecycleManager()
        if args.run_container is not None:
            if args.application_id is None:
                logging.info("Missing application-id argument")
                return 1
            if args.context is None:
                logging.info("Missing context argument")
                return 1
            return app_manager_lifecycle_mng.run_container(
                args.run_container, args.application_id, args.context
            )
        elif args.stop_container is not None:
            if args.timeout is None:
                logging.info("Missing timeout argument")
                return 1
            return app_manager_lifecycle_mng.stop_container(
                args.stop_container, args.timeout
            )
        elif args.kill_container:
            return app_manager_lifecycle_mng.kill_container(
                args.kill_container
            )
    except Exception:
        return 1


if __name__ == "__main__":
    sys.exit(_main())
