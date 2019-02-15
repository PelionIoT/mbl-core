# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Container abstraction."""

import json
import os
import subprocess
from enum import Enum

from .utils import log


DEFAULT_CONTAINER_LOG_DIR = os.path.join(os.sep, "var", "log", "app")


class ContainerState(Enum):
    """Existing OCI container status."""

    CREATED = "created"
    RUNNING = "running"
    STOPPED = "stopped"
    DOES_NOT_EXIST = "does not exist"
    UNRECOGNIZED = "unrecognized"
    ALREADY_STOPPED = "process already finished"
    NOT_RUNNING = "not running"


def get_state(container_id):
    """Get the status of an existing container.

    The container status is retrieved from the status information returned
    via stdout from running the command `runc state <container_id>`. The
    status information is a JSON string which contains a JSON attribute
    named 'status'.

    Return a ContainerState object.
    """
    log.debug("Get the state info for container '{}'".format(container_id))
    # Command syntax:
    # `runc state <container_id>`
    cmd = ["runc", "state", container_id]
    try:
        process = subprocess.run(
            cmd,
            check=True,
            stdin=None,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as error:
        err_output = error.stdout.decode("utf-8")
        if ContainerState.DOES_NOT_EXIST.value in err_output:
            return ContainerState.DOES_NOT_EXIST

        msg = "Getting state for container '{}' failed, error: {}".format(
            container_id, err_output
        )
        raise ContainerStateError(msg)

    decoded_proc_output = process.stdout.decode("utf-8")
    try:
        state_info_json_doc = json.loads(decoded_proc_output)
    except json.JSONDecodeError as error:
        msg = "Deserializing '{}' state info failed, error: {}".format(
            container_id, str(error)
        )
        raise ContainerStateError(msg)

    if "status" not in state_info_json_doc:
        msg = (
            "No 'status' found attribute in container '{}' state info.\n"
            " JSON doc:\n'{}'".format(container_id, decoded_proc_output)
        )
        raise ContainerStateError(msg)

    try:
        # return the enum member associated with the value of the
        # 'status' JSON attribute found
        return ContainerState(state_info_json_doc["status"])
    except ValueError:
        log.error(
            "Unrecognized status '{}' from container '{}'".format(
                state_info_json_doc["status"], container_id
            )
        )
        return ContainerState.UNRECOGNIZED


def create(container_id, bundle_path):
    """Create a container.

    Check that a container with the specified id does not already exists
    before attempting to create a new container.
    Normally we could just run a command and if it fails, determine why
    based on the return code and stdout of the command. In this case, it is
    not possible to capture stdout of `runc create` because `runc` will let
    the container it creates inherit the stdio for `runc`, so attempting to
    capture `runc`'s output will also result in capturing the containers
    output.
    Instead, a container existence is first checked by attempting to fetch
    its state.
    """
    log.debug(
        "Create container with id '{}' from '{}' bundle".format(
            container_id, bundle_path
        )
    )
    container_state = get_state(container_id)

    if container_state != ContainerState.DOES_NOT_EXIST:
        # a container with the specified id already exists, do not
        # attempt to create another one
        msg = "Container with id '{}' already exists".format(container_id)
        raise ContainerAlreadyExistError(msg)

    log.debug(
        "Container with id '{}' does not exists, create one.".format(
            container_id
        )
    )

    if not os.path.isdir(bundle_path):
        msg = (
            "No bundle for '{}' found at '{}',"
            " the directory does not exist".format(container_id, bundle_path)
        )
        raise ContainerCreationError(msg)

    with ContainerLogFile(container_id) as container_log:
        # set the container log file stream to subprocess.DEVNULL
        # if unable to open a stream. This is done because
        # ContainerLogFile does not throw an exception on failure to
        # open a file. Failure to open a container log file should not
        # result in a failure to create a container.
        container_log = container_log or subprocess.DEVNULL

        log.debug("Creating container with id '{}'".format(container_id))
        # Command syntax:
        # `runc create --bundle <bundle_path> <container_id>`
        cmd = ["runc", "create", "--bundle", bundle_path, container_id]
        log.debug("Execute command: '{}'".format(" ".join(cmd)))

        try:
            subprocess.run(
                cmd,
                check=True,
                stdin=subprocess.DEVNULL,
                stdout=container_log,
                stderr=container_log,
            )
        except subprocess.CalledProcessError as error:
            err_output = error.stdout.decode("utf-8")
            msg = "Creating container '{}' failed, error: {}".format(
                container_id, bundle_path, err_output
            )
            raise ContainerCreationError(msg)

        log.info(
            "Container '{}' created from '{}'".format(
                container_id, bundle_path
            )
        )


def start(container_id):
    """Execute a user defined process in a created container."""
    log.debug("Start container with id '{}'".format(container_id))
    # Command syntax:
    # `runc start <container_id>`
    cmd = ["runc", "start", container_id]
    log.debug("Execute command: '{}'".format(" ".join(cmd)))
    try:
        subprocess.run(
            cmd,
            check=True,
            stdin=None,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as error:
        err_output = error.stdout.decode("utf-8")
        msg = "Starting container '{}' failed, error: '{}'".format(
            container_id, err_output
        )
        raise ContainerStartError(msg)

    log.info("Container '{}' started".format(container_id))


def kill(container_id, signal="SIGTERM"):
    """Send the specified signal to a container's init process."""
    log.debug(
        "Kill container '{}' with signal '{}'".format(container_id, signal)
    )
    # Command syntax:
    # `runc kill <container_id> <signal>`
    cmd = ["runc", "kill", container_id, signal]
    log.debug("Execute command: '{}'".format(" ".join(cmd)))
    try:
        subprocess.run(
            cmd,
            check=True,
            stdin=None,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as error:
        err_output = error.stdout.decode("utf-8")
        msg = "Killing container '{}' failed, error: '{}'".format(
            container_id, err_output
        )
        raise ContainerKillError(msg)

    log.debug(
        "Container '{}' killed with signal '{}'".format(container_id, signal)
    )


def delete(container_id):
    """Delete any resources held by the container."""
    log.debug("Delete container with id '{}'".format(container_id))
    # Command syntax:
    # `runc delete <container_id>`
    cmd = ["runc", "delete", container_id]
    log.debug("Execute command: '{}'".format(" ".join(cmd)))
    try:
        subprocess.run(
            cmd,
            check=True,
            stdin=None,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as error:
        err_output = error.stdout.decode("utf-8")
        msg = "Deleting container '{}' failed, error: '{}'".format(
            container_id, err_output
        )
        raise ContainerDeleteError(msg)

    log.debug("Container '{}' deleted".format(container_id))


class ContainerLogFile:
    """
    Encapsulate opening of container log files.

    The point of having this class rather than using plain "open" is that:
    * It doesn't throw exceptions on failure. If we can't open a container's
      log file, just log an error and carry on.
    * It creates the log directory if it doesn't already exist.

    ContainerLogFile is really a "context manager" class (to be used in "with"
    statements).
    """

    def __init__(
        self, container_id, container_log_dir=DEFAULT_CONTAINER_LOG_DIR
    ):
        """Create object to be used in "with" statement."""
        self.container_id = container_id
        self.container_log_dir = container_log_dir
        self.log_file = None
        self.log_file_path = os.path.join(
            self.container_log_dir, "{}.log".format(container_id)
        )

    def __enter__(self):
        """
        Open the container log file.

        The log file directory is created if it does not already exists before
        opening the file for writing by appending to the end of the file if it
        exists.

        Return a `file object` for the container log file iff it has been
        successfully opened. Otherwise return `None`
        """
        try:
            os.makedirs(self.container_log_dir, exist_ok=True)
        except OSError as error:
            log.error(
                "Failed to create container log directory {}".format(
                    self.container_log_dir, error
                )
            )
            return

        try:
            self.log_file = open(self.log_file_path, "a")
            return self.log_file
        except OSError as error:
            log.error(
                "Failed to open log file {} for container {}".format(
                    self.log_file_path, self.container_id, error
                )
            )
            return

    def __exit__(self, type, value, tb):
        """Close the container log file."""
        if self.log_file:
            self.log_file.close()
        return False


class ContainerStateError(Exception):
    """An exception for a failure to obtain a container state information."""


class ContainerAlreadyExistError(Exception):
    """An exception to indicate that a container already exists.

    It is thrown whenever requested to create a container but one with the
    specified id already exists.
    """


class ContainerCreationError(Exception):
    """An exception to indicate a failure during the creation of a container.

    It is thrown whenever the subprocess to start a container fails.
    """


class ContainerStartError(Exception):
    """An exception to indicate a failure to start a container."""


class ContainerKillError(Exception):
    """An exception to indicate a failure during the killing of a container.

    It is thrown whenever the subprocess to kill a container fails.
    """


class ContainerDeleteError(Exception):
    """An exception to indicate a failure during the deletion of a container.

    It is thrown whenever the subprocess to deletre a container fails.
    """
