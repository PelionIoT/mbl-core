# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Application Package installation."""

import os
import shutil
import subprocess

from mbl.app_lifecycle_manager.container import (
    OCI_BUNDLE_CONFIGURATION,
    OCI_BUNDLE_FILESYSTEM,
)

from .parser import parse_app_info, AppInfoParserError
from .utils import log


class AppManager(object):
    """
    Responsible for installing and removing application.

    An application is a OPKG package with an `.ipk` extension.
    """

    def __init__(self):
        """Create an app manager."""
        # When opkg installs new package while "D" environment variable
        # is defined, opkg will not call ldconfig in rootfs partition
        os.environ["D"] = "1"

    # ---------------------------- Public Methods -----------------------------

    def get_app_name(self, app_pkg):
        """Get the name of an application from the package meta data."""
        log.debug("Getting app name from pkg '{}'".format(app_pkg))
        app_name = ""
        try:
            app_name = parse_app_info(app_pkg)["Package"]
        except AppInfoParserError as error:
            msg = "Getting the app name from '{}' failed, error: {}".format(
                app_pkg, str(error)
            )
            raise AppIdError(msg)
        else:
            log.info("Application name is '{}'".format(app_name))
            return app_name

    def install_app(self, app_pkg, app_path):
        """Install an application.

        The application is installed at a registered destination so the package
        manager is able to install multiple version of the application, as
        long as the registration path is different.
        """
        log.debug("installing package '{}' to '{}'".format(app_pkg, app_path))
        # Command syntax:
        # opkg --add-dest <app_name>:<app_path> install <app_pkg>
        app_name = self.get_app_name(app_pkg)
        dest = "{}:{}".format(app_name, app_path)
        cmd = ["opkg", "--add-dest", dest, "install", app_pkg]
        log.debug("Execute command: {}".format(" ".join(cmd)))
        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as error:
            err_output = error.stdout.decode("utf-8")
            msg = "Installing '{}' at '{}' failed, error: '{}'".format(
                app_pkg, app_path, err_output
            )
            raise AppInstallError(msg)
        else:
            log.info(
                "'{}' from '{}' package successfully installed at '{}'".format(
                    app_name, app_pkg, app_path
                )
            )

    def remove_app(self, app_name, app_path):
        """Remove an application.

        The application found at the registered destination is deleted.
        """
        log.debug("Removing app '{}' from '{}'".format(app_name, app_path))
        if not os.path.isdir(app_path):
            msg = "The application path '{}' does not exist".format(app_path)
            raise AppPathInexistent(msg)
        # Command syntax:
        # opkg --add-dest <app_name>:<app_path> remove <app_name>
        dest = "{}:{}".format(app_name, app_path)
        cmd = ["opkg", "--add-dest", dest, "remove", app_name]
        log.debug("Execute command: {}".format(" ".join(cmd)))
        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as error:
            err_output = error.stdout.decode("utf-8")
            msg = "Removing '{}' at '{}' failed, error: {}".format(
                app_name, app_path, err_output
            )
            raise AppUninstallError(msg)
        else:
            shutil.rmtree(app_path)
            app_parent_dir = os.path.dirname(app_path)
            if not os.listdir(app_parent_dir):
                shutil.rmtree(app_parent_dir)
            log.info(
                "'{}' removal from '{}' successful".format(app_name, app_path)
            )

    def force_install_app(self, app_pkg, app_path):
        """
        Force-install an application.

        Remove a previous installation of the app at a specified location.
        """
        if os.path.isdir(app_path):
            app_name = self.get_app_name(app_pkg)
            self.remove_app(app_name, app_path)
        self.install_app(app_pkg, app_path)

    def list_installed_apps(self, apps_path):
        """List all installed applications.

        Each application is supposed to have a directory within `apps_path`.
        Each application directory is named after the installed application.
        """
        log.debug("List of installed applications:\n")
        for dirpath, dirnames, filenames in os.walk(apps_path):
            # an application version directory must have both components
            if (
                OCI_BUNDLE_FILESYSTEM in dirnames
                and OCI_BUNDLE_CONFIGURATION in filenames
            ):
                app_name = os.path.basename(os.path.dirname(dirpath))
                # Command syntax:
                # opkg --add-dest <app_name>:<dirpath> list-installed
                dest = "{}:{}".format(app_name, dirpath)
                cmd = ["opkg", "--add-dest", dest, "list-installed"]
                try:
                    subprocess.check_call(cmd)
                except subprocess.CalledProcessError as error:
                    err_output = error.stdout.decode("utf-8")
                    msg = (
                        "Listing installed apps at '{}' failed,"
                        " error: '{}'".format(
                            os.path.join(apps_path, app_name), err_output
                        )
                    )
                    raise AppListingError(msg)


class AppIdError(Exception):
    """An exception for a missing application name."""


class AppInstallError(Exception):
    """An exception for an app installation failure."""


class AppUninstallError(Exception):
    """An exception for an app uninstallation failure."""


class AppPathInexistent(Exception):
    """An exception raised when an application path is inexistent."""


class AppListingError(Exception):
    """An exception for a failure to list installed apps."""
