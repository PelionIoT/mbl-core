# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Application Package installation."""

import os
import shutil
import subprocess
from pathlib import Path

from mbl.app_lifecycle_manager.container import get_oci_bundle_paths

from .parser import parse_app_info, AppInfoParserError
from .utils import log


APPS_PATH = os.path.join(os.sep, "home", "app")


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
            subprocess.run(
                cmd,
                check=True,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except subprocess.CalledProcessError as error:
            msg = "Installing '{}' at '{}' failed, error: '{}'".format(
                app_pkg,
                app_path,
                error.stdout.decode() if error.stdout else error.returncode,
            )
            # perform cleanup if the application was partly installed
            if os.path.exists(app_path):
                self.remove_app(app_name, app_path)
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
            subprocess.run(
                cmd,
                check=True,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except subprocess.CalledProcessError as error:
            msg = "Removing '{}' at '{}' failed, error: {}".format(
                app_name,
                app_path,
                error.stdout.decode() if error.stdout else error.returncode,
            )
            raise AppUninstallError(msg)
        else:
            app_parent_dir = str(Path(app_path).resolve().parent)
            shutil.rmtree(app_path)
            if not os.listdir(app_parent_dir) and app_parent_dir != APPS_PATH:
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
        app_bundles = get_oci_bundle_paths(apps_path)
        for app_bundle in app_bundles:
            app_name = os.path.basename(os.path.dirname(app_bundle))
            # Command syntax:
            # opkg --add-dest <app_name>:<app_bundle> list-installed
            dest = "{}:{}".format(app_name, app_bundle)
            cmd = ["opkg", "--add-dest", dest, "list-installed"]
            try:
                subprocess.run(
                    cmd,
                    check=True,
                    stdin=None,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                )
            except subprocess.CalledProcessError as error:
                msg = (
                    "Listing installed apps at '{}' failed,"
                    " error: '{}'".format(
                        os.path.join(apps_path, app_name),
                        error.stdout.decode()
                        if error.stdout
                        else error.returncode,
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
