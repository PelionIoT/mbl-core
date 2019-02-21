# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Package unpacking and application installation."""

import json
import os
import subprocess
import sys
import tarfile
from collections import namedtuple

from .utils import log, human_sort
import mbl.app_manager.manager as apm
import mbl.app_lifecycle_manager.manager as alm
import mbl.app_lifecycle_manager.container as alc


IPKS_EXCTRACTION_PATH = os.path.join(os.sep, "mnt", "cache", "opkg", "src_ipk")
APPS_INSTALLATION_PATH = os.path.join(os.sep, "home", "app")

NEW_BUNDLE_PATH = "new_bundle_path"
CURRENT_BUNDLE_PATH = "cur_bundle_path"

ROLLBACK_EXCEPTIONS = (
    alc.ContainerKillError,
    alc.ContainerDeleteError,
    apm.AppPathInexistent,
    apm.AppUninstallError,
    alc.AppStopTimeoutError,
    alc.ContainerCreationError,
    alc.ContainerStartError,
)


class AppUpdateManager:
    """
    Responsible for handling application package.

    An update package is a tar archive that can contain one or multiple ipks.
    Each application found in an ipk can also be installed and run.
    """

    def __init__(self, update_pkg):
        """Create an app package handler."""
        self._ipks = []
        self._pkg_arch_members = []
        self._installed_apps = []
        self.update_pkg = update_pkg
        self.app_mng = apm.AppManager()

    # ---------------------------- Public Methods -----------------------------

    def unpack(self):
        """Unpack the ipk(s) from the update package."""
        log.info("Unpacking '{}'".format(self.update_pkg))

        if not tarfile.is_tarfile(self.update_pkg):
            msg = "Package '{}' is not a tar file".format(self.update_pkg)
            raise IllegalPackage(msg)

        with tarfile.open(self.update_pkg) as tar_file:
            self._get_ipks_from_pkg(tar_file)
            if not self._ipks:
                msg = "No 'ipk' file found in package '{}'".format(
                    self.update_pkg
                )
                raise IllegalPackage(msg)

            try:
                tar_file.extractall(
                    path=IPKS_EXCTRACTION_PATH, members=self._pkg_arch_members
                )
            except tarfile.TarError as error:
                msg = "Unarchiving package '{}' failed, error: {}".format(
                    self.update_pkg, str(error)
                )
                raise IllegalPackage(msg)

            log.info(
                "Update package '{}' successfully unpacked".format(
                    self.update_pkg
                )
            )

    def install_apps(self):
        """Install application(s) from ipk(s)."""
        if not self._ipks:
            return

        for ipk in self._ipks:
            ipk_path = os.path.join(IPKS_EXCTRACTION_PATH, ipk)
            try:
                self._manage_app_installation(ipk_path)
            except (apm.AppIdError, apm.AppInstallError) as install_error:
                log.error(
                    "Failed to install '{}', error: '{}'".format(
                        ipk, str(install_error)
                    )
                )
                try:
                    # some apps may have been successfully installed,
                    # remove them
                    self._remove_apps_bundles(NEW_BUNDLE_PATH)
                except apm.AppUninstallError as uninstall_error:
                    # TODO: handle failure to remove new version
                    raise uninstall_error from install_error
                else:
                    raise install_error

    def start_installed_apps(self):
        """Run applications that have been successfully installed if any.

        Rollback to previous version of applications in case of failure to
        run the new version of the app.
        """
        if not self._installed_apps:
            return

        log.info(
            "Run installed app(s): {}".format(
                [app.name for app in self._installed_apps]
            )
        )

        self._stop_running_versions()
        self._run_new_installed_versions()
        self._remove_previous_versions()

    # --------------------------- Private Methods -----------------------------

    def _validate_archive_member(self, tar_info):
        """Validate the tar archive member."""
        log.debug("Check that '{}' is a valid ipk".format(tar_info.name))

        if not tar_info.name.lower().endswith(".ipk"):
            msg = (
                "IPK(s) expected to be found in package '{}'"
                " however '{}' was found".format(
                    self.update_pkg, tar_info.name
                )
            )
            raise IllegalPackage(msg)
        elif not tar_info.isfile():
            msg = "'{}' found in package '{}' isn't regular file".format(
                tar_info.name, self.update_pkg
            )
            raise IllegalPackage(msg)
        elif os.path.dirname(tar_info.name):
            msg = (
                "Directories are not expected to be found in package '{}'"
                " however '{}' was found".format(
                    self.update_pkg, tar_info.name
                )
            )
            raise IllegalPackage(msg)

        log.info("'{}' is a valid ipk".format(tar_info.name))

    def _get_ipks_from_pkg(self, tar_file):
        """Retrieve ipks contained in a package."""
        for tar_info in tar_file:
            log.info(
                "Found archive member '{}' in '{}' archive".format(
                    tar_info.name, self.update_pkg
                )
            )
            self._validate_archive_member(tar_info)
            self._ipks.append(tar_info.name)
            self._pkg_arch_members.append(tar_info)

    def _install_app(self, app_record):
        """Install an application from an ipk."""
        log.info(
            "Installing '{}' from '{}' to '{}'".format(
                app_record.name,
                app_record.ipk_path,
                app_record.new_bundle_path,
            )
        )
        self.app_mng.install_app(
            app_record.ipk_path, app_record.new_bundle_path
        )
        self._installed_apps.append(app_record)

    def _manage_app_installation(self, ipk_path):
        """Manage the process of installing application from ipk."""
        log.debug("Installing app from '{}'".format(ipk_path))
        app_name = self.app_mng.get_app_name(ipk_path)
        cur_bundle_path, new_bundle_path = self._get_app_bundle_paths(app_name)
        app_record = AppRecord(
            name=app_name,
            cur_bundle_path=cur_bundle_path,
            new_bundle_path=new_bundle_path,
            ipk_path=ipk_path,
        )
        self._install_app(app_record)

    def _rollback_apps(self):
        """Revert to previous installation of applications.

        Remove the newly installed versions of applications and restart the
        previously installed versions.
        Remove the only present version of an application if it was not
        previously installed.
        """
        log.debug(
            "Roll back installation(s) of '{}'".format(
                [app.name for app in self._installed_apps]
            )
        )
        self._stop_all_newly_installed_apps()
        self._remove_apps_bundles(NEW_BUNDLE_PATH)
        self._run_previous_app_versions()

    def _get_app_bundle_paths(self, app_name):
        """Return the current and next paths for an app installation."""
        log.debug("Get the current and next paths for an app installation")
        app_parent_dir = os.path.join(APPS_INSTALLATION_PATH, app_name)
        if not os.path.isdir(app_parent_dir):
            return None, os.path.join(app_parent_dir, "0")

        app_version_dirs = alc.get_oci_bundle_paths(app_parent_dir)

        if app_version_dirs:
            human_sort(app_version_dirs)
            current_path = app_version_dirs[0]  # is that correct?
            log.debug("current_path = '{}'".format(current_path))
            latest_app_version = os.path.basename(app_version_dirs[-1])
            next_path = os.path.join(
                app_parent_dir, str((int(latest_app_version) + 1))
            )
            log.debug("next_path = '{}'".format(next_path))
            return current_path, next_path

    def _remove_apps_bundles(self, path_attr):
        """Remove applications current or previous bundle.

        The bundle to remove is indicated by `path_attr` which must be set to
        `CURRENT_BUNDLE_PATH` or `NEW_BUNDLE_PATH`.
        """
        if not self._installed_apps:
            return

        log.debug(
            "Remove version(s) of '{}'".format(
                [app.name for app in self._installed_apps]
            )
        )
        for app in self._installed_apps:
            if not getattr(app, path_attr):
                log.debug(
                    "No app bundle path to remove specified for '{}'".format(
                        app.name
                    )
                )
                continue
            self.app_mng.remove_app(app.name, getattr(app, path_attr))

    def _stop_running_versions(self):
        """Stop running version of application(s)."""
        for app in self._installed_apps:
            log.info(
                "Stop running instance of '{}' if any"
                " exist before running the new version".format(app.name)
            )
            try:
                alm.terminate_app(app.name)
            except (
                alc.ContainerKillError,
                alc.ContainerDeleteError,
                alc.AppStopTimeoutError,
            ) as terminate_error:
                log.error(
                    "Rollback applications as failed"
                    " to stop '{}'".format(app.name)
                )
                try:
                    self._rollback_apps()
                except ROLLBACK_EXCEPTIONS as rollback_error:
                    raise rollback_error from terminate_error
                else:
                    raise terminate_error

    def _stop_all_newly_installed_apps(self):
        """Stop all applications installed during the update."""
        for app in self._installed_apps:
            try:
                alm.terminate_app(app.name)
            except (
                alc.ContainerKillError,
                alc.ContainerDeleteError,
                alc.AppStopTimeoutError,
            ) as terminate_error:
                if alc.ContainerState.DOES_NOT_EXIST.value in str(
                    terminate_error
                ):
                    # Not all new app versions may have been started
                    # Carry on stopping some that may have been started already
                    pass
                else:
                    # TODO: handle failure to stop new version during rollback
                    raise terminate_error

    def _run_new_installed_versions(self):
        """Run newly installed application(s) version."""
        for app in self._installed_apps:
            log.info(
                "Run '{}' from '{}'".format(app.name, app.new_bundle_path)
            )
            try:
                alm.run_app(app.name, app.new_bundle_path)
            except (
                alc.ContainerCreationError,
                alc.ContainerStartError,
            ) as run_error:
                log.error(
                    "Rollback applications as failed to start"
                    "'{}' from '{}'".format(app.name, app.new_bundle_path)
                )
                try:
                    self._rollback_apps()
                except ROLLBACK_EXCEPTIONS as rollback_error:
                    raise rollback_error from run_error
                else:
                    raise run_error

    def _run_previous_app_versions(self):
        """Run previous versions of apps installed during the upgrade."""
        for app in self._installed_apps:
            if not app.cur_bundle_path:
                log.debug(
                    "'{}' not previously installed,"
                    " nothing to rollback to.".format(app.name)
                )
                continue
            try:
                alm.run_app(app.name, app.cur_bundle_path)
            except (
                alc.ContainerCreationError,
                alc.ContainerStartError,
            ) as run_error:
                log.error(
                    "Failed to restart '{}' from '{}'".format(
                        app.name, app.cur_bundle_path
                    )
                )
                # TODO: handle failure to restart old version
                raise run_error
            else:
                log.info(
                    "'{}' was restarted from '{}'".format(
                        app.name, app.cur_bundle_path
                    )
                )

    def _remove_previous_versions(self):
        """Remove previous application(s) installation."""
        try:
            self._remove_apps_bundles(CURRENT_BUNDLE_PATH)
        except (apm.AppPathInexistent, apm.AppUninstallError) as error:
            # TODO: handle failure to remove old version
            raise error


class IllegalPackage(Exception):
    """An exception for package containing incorrect data."""


# Record of application
AppRecord = namedtuple(
    "AppRecord", ("name", CURRENT_BUNDLE_PATH, NEW_BUNDLE_PATH, "ipk_path")
)
