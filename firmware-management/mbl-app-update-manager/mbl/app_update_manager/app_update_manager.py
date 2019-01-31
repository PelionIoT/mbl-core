#!/usr/bin/env python3
# Copyright (c) 2018 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""This script manage application updates."""

import sys
import os
import subprocess
import logging
import tarfile
from enum import Enum
import mbl.app_manager.app_manager as apm
import mbl.app_lifecycle_manager.app_lifecycle_manager as alm

__version__ = "1.0"
APP_STOP_TIMEOUT = 3
SRC_IPK_PATH = "/mnt/cache/opkg/src_ipk"


class Error(Enum):
    """AppUpdateManager error codes."""

    SUCCESS = 0
    ERR_OPERATION_FAILED = 1
    ERR_INSTALL_PACKAGE_FAILED = 2
    ERR_REMOVE_PACKAGE_FAILED = 3
    ERR_STOP_CONTAINER_FAILED = 4
    ERR_RUN_CONTAINER_FAILED = 5
    ERR_GET_APP_ID_FAILED = 6
    ERR_CONTAINER_DOES_NOT_EXIST = 7
    ERR_INVALID_ARGS = 8
    ERR_TAR_FAILURE = 9


class AppUpdateManager:
    """Manage application Updates."""

    def __init__(self):
        """Initialize AppUpdateManager class."""
        self.logger = logging.getLogger("AppUpdateManager")
        self.logger.info(
            "Creating AppUpdateManager version {}".format(__version__)
        )
        self.app_mng = apm.AppManager()
        self.app_lifecycle_mng = alm.AppLifecycleManager()

    def stop_container(self, app_id, timeout):
        """
        Stop container using AppLifecycleManager.

        :param app_id: App id
        :param timeout: Timeout
        :return:
                Error.SUCCESS
                Error.ERR_CONTAINER_DOES_NOT_EXIST
                Error.ERR_STOP_CONTAINER_FAILED
        """
        self.logger.debug("Stop Container id {}".format(app_id))
        result = self.app_lifecycle_mng.stop_container(app_id, timeout)
        if result is alm.Error.ERR_CONTAINER_DOES_NOT_EXIST:
            self.logger.debug("Container id {} does not exist".format(app_id))
            return Error.ERR_CONTAINER_DOES_NOT_EXIST
        if result is not alm.Error.SUCCESS:
            self.logger.error(
                "Stop container id {} failed with err: {}".format(
                    app_id, result
                )
            )
            return Error.ERR_STOP_CONTAINER_FAILED
        self.logger.debug("Stop Container id {} succeeded.".format(app_id))
        return Error.SUCCESS

    def run_container(self, app_id):
        """
        Run container using AppLifecycleManager.

        :param app_id: App id
        :return:
                Error.SUCCESS
                Error.ERR_RUN_CONTAINER_FAILED
        """
        self.logger.debug("Run Container id {}".format(app_id))
        result = self.app_lifecycle_mng.run_container(app_id, app_id)
        if result != alm.Error.SUCCESS:
            self.logger.error(
                "Run container id {} failed with err: {}".format(
                    app_id, result
                )
            )
            return Error.ERR_RUN_CONTAINER_FAILED
        self.logger.debug("Run Container id {} succeeded.".format(app_id))
        return Error.SUCCESS

    def install_package(self, package_path):
        """
        Install package using AppManager.

        :param package_path: Package path to IPK file
        :return:
                Error.SUCCESS
                Error.ERR_INSTALL_PACKAGE_FAILED
        """
        self.logger.debug("Install package {}".format(package_path))
        try:
            self.app_mng.install_package(package_path)
        except subprocess.CalledProcessError as e:
            self.logger.exception(
                "Install package {} failed with error: {}".format(
                    package_path, e.returncode
                )
            )
            return Error.ERR_INSTALL_PACKAGE_FAILED
        self.logger.debug("Install package {} succeeded".format(package_path))
        return Error.SUCCESS

    def remove_package(self, package_name):
        """
        Remove package using AppManager.

        :param package_name: Package name
        :return:
                Error.SUCCESS
                Error.ERR_REMOVE_PACKAGE_FAILED
        """
        self.logger.debug("Remove package {}".format(package_name))
        try:
            self.app_mng.remove_package(package_name)
        except subprocess.CalledProcessError as e:
            self.logger.exception(
                "Remove package {} failed with error: {}".format(
                    package_name, e.returncode
                )
            )
            return Error.ERR_REMOVE_PACKAGE_FAILED
        self.logger.debug("Remove package {} succeeded".format(package_name))
        return Error.SUCCESS

    def get_app_id_from_package(self, package_path):
        """
        Get App id from package using AppManager.

        :param package_path: Package path
        :return:
                Error.SUCCESS, app id
                Error.ERR_GET_APP_ID_FAILED, empty app id
        """
        self.logger.debug("Get app id from package {}".format(package_path))
        app_id = ""
        try:
            # App id is the destination directory of a package
            app_id = self.app_mng.get_package_dest_dir(package_path)
        except subprocess.CalledProcessError as e:
            self.logger.exception(
                "Get App id failed with error: {}".format(e.returncode)
            )
            return Error.ERR_GET_APP_ID_FAILED, app_id
        self.logger.debug("App Id is {}".format(app_id))
        return Error.SUCCESS, app_id


def install_package_and_run_container(package_path):
    """
    Install package and run container.

    If application exists it will first stop and remove it, and then
    Install and run it.
    :param package_path: Package path
    :return:
            Error.SUCCESS
            Error.ERR_INSTALL_PACKAGE_FAILED
            Error.ERR_REMOVE_PACKAGE_FAILED
            Error.ERR_STOP_CONTAINER_FAILED
            Error.ERR_RUN_CONTAINER_FAILED
            Error.ERR_GET_APP_ID_FAILED
            Error.ERR_CONTAINER_DOES_NOT_EXIST
    """
    logging.info("Install package and run container: {}".format(package_path))
    app_update_manager = AppUpdateManager()

    # Get App id from package
    result, app_id = app_update_manager.get_app_id_from_package(package_path)
    if result != Error.SUCCESS:
        return result
    # Stop container
    result = app_update_manager.stop_container(app_id, APP_STOP_TIMEOUT)
    # It is fine if container does not exist...nothing to stop
    if (
        result != Error.SUCCESS and 
            result != Error.ERR_CONTAINER_DOES_NOT_EXIST
    ):
        return result
    # Remove package (app id is the package name)
    result = app_update_manager.remove_package(app_id)
    if result != Error.SUCCESS:
        return result
    # Install package
    result = app_update_manager.install_package(package_path)
    if result != Error.SUCCESS:
        return result
    # Run container (we use app id as container id)
    result = app_update_manager.run_container(app_id)
    if result != Error.SUCCESS:
        return result
    return Error.SUCCESS


def extract_ipks_from_tar(tar_path):
    """
    Extract .ipk files from a tar file.

    Returns a 2-tuple (ipk_paths, error) where ipk_paths is a list of paths to
    extracted .ipk files and error is one of:
        Error.SUCCESS
        Error.ERR_TAR_FAILURE
    """
    logging.info(
        "Extracting .ipk files from update payload tar file {}".format(
            tar_path
        )
    )
    try:
        ipk_paths = []
        ipk_members = []
        with tarfile.open(tar_path) as tar:
            search_for_ipks_in_tar(tar_path, tar, ipk_paths, ipk_members)
            tar.extractall(path=SRC_IPK_PATH, members=ipk_members)
        if not ipk_paths:
            logging.info(
                "No .ipk files found in update payload tar file {}".format(
                    tar_path
                )
            )
        return (ipk_paths, Error.SUCCESS)
    except tarfile.TarError:
        logging.exception(
            "Failed to extract .ipk files from tar archive {}".format(tar_path)
        )
        return ([], Error.ERR_TAR_FAILURE)
    except Exception:
        logging.exception("Invalid tar archive {} content".format(tar_path))
        return ([], Error.ERR_TAR_FAILURE)


def search_for_ipks_in_tar(tar_path, tar, ipk_paths, ipk_members):
    """Search for ipks within a tar file."""
    for tar_info in tar:
        logging.debug(
            "Processing file {} in update payload tar file {}".format(
                tar_info.name, tar_path
            )
        )
        if not tar_info.name.lower().endswith(".ipk"):
            raise Exception(
                "Tar archive {} includes non-IPK file: {}".format(
                    tar_path, tar_info.name
                )
            )
        if not tar_info.isfile():
            raise Exception(
                "IPK file {} from update payload tar file {} - is not"
                "a regular file".format(tar_info.name, tar_path)
            )
        if os.path.dirname(tar_info.name):
            raise Exception(
                "IPK file {} from update payload tar file {} has directory"
                " components".format(tar_info.name, tar_path)
            )
        ipk_members.append(tar_info)
        ipk_paths.append(os.path.join(SRC_IPK_PATH, tar_info.name))
        logging.info(
            "Found .ipk file {} in update payload tar file {}".format(
                tar_info.name, tar_path
            )
        )


def install_and_run_apps_from_tar(tar_path):
    """Install and run application contained in a tar file."""
    ipk_paths, ret = extract_ipks_from_tar(tar_path)
    if ret != Error.SUCCESS:
        return ret

    for ipk_path in ipk_paths:
        ret = install_package_and_run_container(ipk_path)
        if ret != Error.SUCCESS:
            return ret
    return Error.SUCCESS
