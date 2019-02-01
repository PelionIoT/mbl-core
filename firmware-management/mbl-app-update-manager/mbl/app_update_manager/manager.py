# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Package unpacking and application installation."""

import os
import subprocess
import sys
import tarfile

from .utils import log
import mbl.app_manager.app_manager as apm
import mbl.app_lifecycle_manager.app_lifecycle_manager as alm


TIMEOUT_STOP_RUNNING_APP_IN_SECONDS = 3
IPKS_EXCTRACTION_PATH = "/mnt/cache/opkg/src_ipk"


class AppUpdateManager:
    """
    Responsible for handling application package.

    A package is a tar archive that can contain one or multiple ipks.
    Each application found in an ipk can also be installed and run.
    """

    def __init__(self, package):
        """Create an app package handler."""
        self._ipks = []
        self._pkg_arch_members = []
        self._installed_app_ids = []
        self._package = package
        self.app_mng = apm.AppManager()
        self.app_lifecycle_mng = alm.AppLifecycleManager()

    # ---------------------------- Public Methods -----------------------------

    def unpack(self):
        """Unpack the ipk(s) from the package.

        Return `True` iff the package has been successfully unpacked and it
        contained only ipks.
        """
        log.info("Unpacking '{}'".format(self._package))

        if not tarfile.is_tarfile(self._package):
            msg = "Package '{}' is not a tar file".format(self._package)
            log.exception(msg)
            raise IllegalPackage(msg)

        try:
            with tarfile.open(self._package) as tar_file:
                self._get_ipks_from_pkg(tar_file)
                if not self._ipks:
                    msg = "No 'ipk' file found in package '{}'".format(
                        self._package
                    )
                    log.exception(msg)
                    raise IllegalPackage(msg)
                tar_file.extractall(
                    path=IPKS_EXCTRACTION_PATH, members=self._pkg_arch_members
                )
                return True

        except tarfile.TarError as error:
            msg = "Unarchiving package '{}' failed with error: {}".format(
                self._package, error
            )
            log.exception(msg)
            raise IllegalPackage(msg)

    def install_apps(self):
        """Install application(s) from ipk(s).

        Return `True` iff all applications in the package are successfully
        installed.
        """
        for ipk in self._ipks:
            if not self._manage_app_installation(
                os.path.join(IPKS_EXCTRACTION_PATH, ipk)
            ):
                return False
        return True

    def start_installed_apps(self):
        """Run applications that have been successfully installed if any."""
        if self._installed_app_ids:
            for app_id in self._installed_app_ids:
                start_app(app_id)

    # --------------------------- Private Methods -----------------------------

    def _validate_archive_member(self, tar_info):
        """Validate the tar archive member.

        Return `True` if the archive member is a regular file with an ipk
        extension.
        """
        log.info("Check that '{}' is a valid ipk".format(tar_info.name))

        if not tar_info.name.lower().endswith(".ipk"):
            msg = (
                "IPK(s) expected to be found in package '{}'"
                " however '{}' was found".format(self._package, tar_info.name)
            )
            log.exception(msg)
            raise IllegalPackage(msg)

        if not tar_info.isfile():
            msg = "'{}' found in package '{}' isn't regular file".format(
                tar_info.name, self._package
            )
            log.exception(msg)
            raise IllegalPackage(msg)

        if os.path.dirname(tar_info.name):
            msg = (
                "Directories are not expected to be found in package '{}'"
                " however '{}' was found".format(self._package, tar_info.name)
            )
            log.exception(msg)
            raise IllegalPackage(msg)

        log.info("'{}' is a valid ipk".format(tar_info.name))
        return True

    def _get_ipks_from_pkg(self, tar_file):
        """Retrieve ipks contained in a package."""
        for tar_info in tar_file:
            log.info(
                "Found archive member '{}' in '{}' archive".format(
                    tar_info.name, self._package
                )
            )

            if self._validate_archive_member(tar_info):
                self._ipks.append(tar_info.name)
                self._pkg_arch_members.append(tar_info)

    def _get_app_id(self, ipk):
        """
        Get the application id from the ipk.

        The application id is also the name of the directory in /home/app
        where the application will be installed.
        """
        log.info("Getting app id from ipk '{}'".format(ipk))

        try:
            app_id = self.app_mng.get_package_dest_dir(ipk)
            log.debug("App Id is '{}'".format(app_id))
            return app_id
        except subprocess.CalledProcessError as e:
            msg = "Getting app id failed with error: {}".format(e.returncode)
            log.exception(msg)
            raise AppOperationError(msg)

    def _start_app(self, app_id):
        """Start an application.

        Return `True` iff an application with the id was started.
        """
        log.info("Starting app with id '{}'".format(app_id))

        ret_code = self.app_lifecycle_mng.run_container(app_id, app_id)

        if ret_code is not alm.Error.SUCCESS:
            msg = "Starting app with id '{}' failed with error: {}".format(
                app_id, ret_code
            )
            log.exception(msg)
            raise AppOperationError(msg)

        log.info("App with id '{}' successfully started".format(app_id))

        return True

    def _stop_app(self, app_id):
        """Stop a running application.

        Return `True` if an application running with the id was stopped or no
        app with the specified id exist.
        """
        log.info("Stopping app with id '{}'".format(app_id))

        ret_code = self.app_lifecycle_mng.stop_app(
            app_id, TIMEOUT_STOP_RUNNING_APP_IN_SECONDS
        )

        if ret_code is not alm.Error.ERR_CONTAINER_DOES_NOT_EXIST:
            # it is ok, the app may not be installed
            log.debug("App with id '{}' does not exist".format(app_id))
        elif ret_code is not alm.Error.SUCCESS:
            msg = "Stopping app with id '{}' failed with error: {}".format(
                app_id, ret_code
            )
            log.exception(msg)
            raise AppOperationError(msg)

        log.info("App with id '{}' successfully stopped".format(app_id))

        return True

    def _remove_app(self, app_id):
        """Remove an application.

        Return `True` iff an application with the specified id was removed.
        """
        log.info("Removing app with id '{}'".format(app_id))

        try:
            self.app_mng.remove_package(app_id)
            log.info("App with id '{}' successfully removed".format(app_id))
            return True
        except subprocess.CalledProcessError as e:
            msg = "Removing app with id '{}' failed with error: {}".format(
                app_id, e.returncode
            )
            log.exception(msg)
            raise AppOperationError(msg)

    def _install_app(self, ipk):
        """
        Install package using AppManager.

        Return `True` iff the application is successfully installed.
        """
        log.info("Installing app from '{}'".format(ipk))

        try:
            self.app_mng.install_package(ipk)
            log.info("App in ipk '{}' successfully installed".format(ipk))
            return True
        except subprocess.CalledProcessError as error:
            msg = "'{}' installation ended with return code: '{}'".format(
                ipk, e.returncode
            )
            log.exception(msg)
            raise AppOperationError(msg)

    def _manage_app_installation(self, ipk):
        """Manage the process of installing application from ipk.

        Return `True` iff application is successfully installed.
        """
        log.info("Installing ipk '{}'".format(ipk))

        try:
            app_id = self._get_app_id(ipk)

            if app_id:
                if self._stop_app(app_id):
                    if self._remove_app(app_id) and self._install_app(ipk):
                        self._installed_app_ids.append(app_id)
                        return True
        except AppOperationError as error:
            msg = "Failure to install '{}', Error: {}".format(ipk, error)
            log.exception(msg)
            raise AppOperationError(msg)


class AppOperationError(Exception):
    """An exception for an app operation failure."""


class IllegalPackage(Exception):
    """An exception for package containing incorrect data."""
