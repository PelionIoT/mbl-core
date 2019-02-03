# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Application Package installation."""

import os
import shutil
import subprocess

from .parser import ApplicationInfoParser
from .utils import log, run_process


class AppManager(object):
    """
    Responsible for installing and removing application.

    An application is a OPKG package with an `.ipk` extension.
    """

    def __init__(self):
        """Create an app manager."""
        # When opkg installs new package while "D" environment variable
        # is defined, opkg will not call ldconfig in rootfs partition
        self._opkg_env = os.environ.copy()
        self._opkg_env["D"] = "1"
        self._pkg_info_parser = ApplicationInfoParser(self._opkg_env)

    # ---------------------------- Public Methods -----------------------------

    def get_app_name(self, app_pkg):
        """Get the name of an application from the package meta data."""
        try:
            log.info("Getting app name from pkg '{}'".format(app_pkg))
            app_name = self._pkg_info_parser.get_pkg_info(app_pkg)["Package"]
            log.debug("Application name is '{}'".format(app_name))
            return app_name
        except subprocess.CalledProcessError as error:
            msg = "Process to get app name failed with error: {}".format(
                error.returncode
            )
            log.exception(msg)
            raise AppIdError(msg)
        except KeyError:
            msg = "'{}' control data does not have a 'Package' field".format(
                app_pkg
            )
            log.exception(msg)
            raise AppIdError(msg)

    def install_app(self, app_pkg, app_path):
        """Install an application.

        The application is installed at a registered destination so the package
        manager is able to install multiple version of the application for as
        long as the registration path is different.
        """
        # Command syntax:
        # opkg --add-dest <app_name>:<app_path> install <app_pkg>
        try:
            log.info(
                "installing package '{}' to '{}'".format(app_pkg, app_path)
            )
            app_name = self.get_app_name(app_pkg)

            dest = "{}:{}".format(app_name, app_path)
            cmd = ["opkg", "--add-dest", dest, "install", app_pkg]
            run_process(command=cmd, env_var=self._opkg_env)
            log.info(
                "'{}' from '{}' package successfully installed at '{}'".format(
                    app_name, app_pkg, app_path
                )
            )
            return True
        except subprocess.CalledProcessError as error:
            msg = "Installing '{}' at '{}' returned code: '{}'".format(
                app_pkg, app_path, error.returncode
            )
            log.exception(msg)
            raise AppInstallError(msg)

    def remove_app(self, app_name, app_path):
        """Remove an application.

        The application found at the registered destination is uninstalled.
        """
        # Command syntax:
        # opkg --add-dest <app_name>:<app_path> remove <app_name>
        try:
            log.info("Removing app '{}' from '{}'".format(app_name, app_path))
            if not os.path.isdir(app_path):
                msg = "The application path '{}' does not exist".format(
                    app_path
                )
                log.exception(msg)
                raise AppUninstallError(msg)

            dest = "{}:{}".format(app_name, app_path)
            cmd = ["opkg", "--add-dest", dest, "remove", app_name]
            run_process(command=cmd, env_var=self._opkg_env)
            log.info(
                "'{}' removal from '{}' successful".format(app_name, app_path)
            )
            shutil.rmtree(app_path)
            return True
        except subprocess.CalledProcessError as error:
            msg = "Removing '{}' at '{}' returned code: '{}'".format(
                app_name, app_path, error.returncode
            )
            log.exception(msg)
            raise AppUninstallError(msg)

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
        log.info("Installed application:\n")
        subdirs = [
            name
            for name in os.listdir(apps_path)
            if os.path.isdir(os.path.join(apps_path, name))
        ]
        for subdir in subdirs:
            # Command syntax:
            # opkg --add-dest <app_name>:<apps_path>/<app_name> list-installed
            app_name = subdir
            dest = "{}:{}".format(app_name, os.path.join(apps_path, app_name))
            cmd = ["opkg", "--add-dest", dest, "list-installed"]
            run_process(command=cmd, env_var=self._opkg_env, log_cmd=False)


class AppIdError(Exception):
    """An exception for a missing application name."""


class AppInstallError(Exception):
    """An exception for an app installation failure."""


class AppUninstallError(Exception):
    """An exception for an app uninstallation failure."""
