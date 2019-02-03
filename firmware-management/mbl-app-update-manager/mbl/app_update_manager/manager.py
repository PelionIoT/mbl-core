# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Package unpacking and application installation."""

import os
import subprocess
import sys
import tarfile

from .utils import log
import mbl.app_manager.manager as apm
import mbl.app_lifecycle_manager.app_lifecycle_manager as alm


TIMEOUT_STOP_RUNNING_APP_IN_SECONDS = 3
IPKS_EXCTRACTION_PATH = "/mnt/cache/opkg/src_ipk"
APPS_INSTALLATION_PATH = "/home/app"


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
        self._installed_app_names = []
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
        if self._installed_app_names:
            for app_name in self._installed_app_names:
                start_app(app_name)

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

    def _start_app(self, app_name):
        """Start an application.

        Return `True` iff an application with the id was started.
        """
        log.info("Starting app '{}'".format(app_name))

        ret_code = self.app_lifecycle_mng.run_container(app_name, app_name)

        if ret_code is not alm.Error.SUCCESS:
            msg = "Starting app '{}' failed with error: {}".format(
                app_name, ret_code
            )
            log.exception(msg)
            raise AppOperationError(msg)

        log.info("App '{}' successfully started".format(app_name))

        return True

    def _stop_app(self, app_name):
        """Stop a running application.

        Return `True` if an application running with the id was stopped or no
        app with the specified id exist.
        """
        log.info("Stopping app '{}'".format(app_name))

        ret_code = self.app_lifecycle_mng.stop_app(
            app_name, TIMEOUT_STOP_RUNNING_APP_IN_SECONDS
        )

        if ret_code is not alm.Error.ERR_CONTAINER_DOES_NOT_EXIST:
            # it is ok, the app may not be installed
            log.debug("App '{}' does not exist".format(app_name))
        elif ret_code is not alm.Error.SUCCESS:
            msg = "Stopping app '{}' failed with error: {}".format(
                app_name, ret_code
            )
            log.exception(msg)
            raise AppOperationError(msg)

        log.info("App '{}' successfully stopped".format(app_name))

        return True

    def _manage_app_installation(self, ipk):
        """Manage the process of installing application from ipk.

        Return `True` iff application is successfully installed.
        """
        log.info("Installing ipk '{}'".format(ipk))

        try:
            app_name = self.app_mng.get_app_name(ipk)

            if app_name:
                if self._stop_app(app_name):
                    if self.app_mng.remove_app(
                        app_name,
                        os.path.join(APPS_INSTALLATION_PATH, app_name),
                    ) and self.app_mng.install_app(
                        ipk, os.path.join(APPS_INSTALLATION_PATH, app_name)
                    ):
                        self._installed_app_names.append(app_name)
                        return True
        except AppOperationError as error:
            msg = "Failure to install '{}', Error: {}".format(ipk, error)
            log.exception(msg)
            raise AppOperationError(msg)


class AppOperationError(Exception):
    """An exception for an app operation failure."""


class IllegalPackage(Exception):
    """An exception for package containing incorrect data."""
