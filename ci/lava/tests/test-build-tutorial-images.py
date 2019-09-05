#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to build images to be used during testing.

Builds:
    user-sample-app-package (also known as HelloWorld) as ipk and tarball
    tarball containg 5 "good" applications.
    tarball containg 4 "good" applications and one with an incorrect
    architecture
    tarball containg 4 "good" applications and one with an oci package error

"""
import fileinput
import os
import pytest
import re
import subprocess

BAD_ARCHITECTURE_IMAGE = "sample-app-3-bad-architecture"
BAD_RUNTIME_IMAGE = "sample-app-4-bad-oci-runtime"

USER_SAMPLE_TAR = "user-sample-app-package_1.0_any.ipk.tar"
MULTI_APP_ALL_GOOD_TAR = "mbl-multi-apps-update-package-all-good.tar"
MULTI_APP_ONE_FAIL_RUN_TAR = "mbl-multi-apps-update-package-one-fail-run.tar"
MULTI_APP_ONE_FAIL_INSTALL_TAR = (
    "mbl-multi-apps-update-package-one-fail-install.tar"
)


class TestBuildTestImages:
    """Class to encapsulate building images for testing."""

    arm_arch = None

    def _replace_line_in_file(self, filename, search, replace):
        """
        Private function to replace lines within a file.

        Used to manipulate the source files to generate images with modified
        content.
        """
        for line in fileinput.FileInput(filename, inplace=1):
            sline = line.strip().split(" ")
            if sline[0].startswith(search):
                sline[1] = replace
            line = " ".join(sline)
            print(line)

    def _remove_first_line_from_file(self, filename):
        """
        Private function to remove the first line in a file.

        Used to manipulate the source files to generate images with modified
        content.
        """
        with open(filename, "r") as fin:
            data = fin.read().splitlines(True)
        with open(filename, "w") as fout:
            fout.writelines(data[1:])

    def test_select_architecture(self, device):
        """Select the architechture based upon the device under test."""
        err = 0

        if (
            device == "bcm2837-rpi-3-b-32"
            or device == "bcm2837-rpi-3-b-plus-32"
            or device == "imx7s-warp-mbl"
            or device == "imx7d-pico-mbl"
            or device == "imx6ul-pico-mbl"
        ):
            TestBuildTestImages.arm_arch = "armv7"
        elif device == "imx8mmevk-mbl":
            TestBuildTestImages.arm_arch = "arm64"
        else:
            print("Unsupported device type {}".format(device))
            err = 1

        assert err == 0

    def test_build_dockcross(self, execute_helper):
        """Build the dockcross compiler."""
        os.chdir("tutorials/helloworld/")

        # Build dockcross
        err, output, error = execute_helper.execute_command(
            [
                "docker",
                "build",
                "-t",
                "linux-" + TestBuildTestImages.arm_arch + ":latest",
                "./cc-env/" + TestBuildTestImages.arm_arch,
            ]
        )

        assert err == 0

    def test_run_dockcross(self, execute_helper):
        """
        Run the dockcross compiler.

        This produces a script that is used to build the images.
        """
        err, output, error = execute_helper.execute_command(
            ["docker", "run", "--rm", "linux-" + TestBuildTestImages.arm_arch]
        )

        if err != 0:
            assert False
            return
        else:
            print("Creating build file")
            build = open("build-{}".format(TestBuildTestImages.arm_arch), "w")
            build.write(output)
            build.close()
            os.chmod("build-{}".format(TestBuildTestImages.arm_arch), 0o777)
            print("Created build file")

        assert err == 0

    def test_build_images(self, execute_helper):
        """Build all of the test images."""
        overall_result = 0

        print("\nCreate User Sample App")
        err, output, error = execute_helper.execute_command(
            [
                "./build-{}".format(TestBuildTestImages.arm_arch),
                "make",
                "release",
            ]
        )

        overall_result += err

        print("Create Good Sample Apps")
        for image in range(1, 6):
            self._replace_line_in_file(
                "src_opkg/CONTROL/control",
                "Package:",
                "sample-app-{}-good".format(image),
            )

            print("\tBuilding sample-app-{}-good".format(image))

            err, output, error = execute_helper.execute_command(
                [
                    "./build-{}".format(TestBuildTestImages.arm_arch),
                    "make",
                    "release",
                ]
            )

            overall_result += err

        print("Create Bad Architecture App")
        self._replace_line_in_file(
            "src_opkg/CONTROL/control", "Package:", BAD_ARCHITECTURE_IMAGE
        )
        self._replace_line_in_file(
            "src_opkg/CONTROL/control", "Version:", "1.1"
        )
        self._replace_line_in_file(
            "src_opkg/CONTROL/control", "Architecture:", "invalid-architecture"
        )

        err, output, error = execute_helper.execute_command(
            [
                "./build-{}".format(TestBuildTestImages.arm_arch),
                "make",
                "release",
            ]
        )
        overall_result += err

        print("Create Bad OCI Runtime App")
        self._replace_line_in_file(
            "src_opkg/CONTROL/control", "Package:", BAD_RUNTIME_IMAGE
        )
        self._replace_line_in_file(
            "src_opkg/CONTROL/control", "Architecture:", "any"
        )
        self._remove_first_line_from_file("src_bundle/config.json")

        err, output, error = execute_helper.execute_command(
            [
                "./build-{}".format(TestBuildTestImages.arm_arch),
                "make",
                "release",
            ]
        )
        overall_result += err

        assert overall_result == 0

    def test_create_tarfiles(
        self, execute_helper, host_tutorials_dir, payload_version
    ):
        """Create the output artifacts."""
        overall_result = 0

        # Navigate to the location of the IPK
        os.chdir("release/ipk/")

        # Create tar bundles
        if not os.path.exists(host_tutorials_dir):
            os.makedirs(host_tutorials_dir)

        # Create payload_version file
        with open("payload_format_version", "w") as file:
            file.write(payload_version)

        print("Copying user-sample-app-package_1.0_any.ipk")

        # User Sample App is needed in both ipk and tar format
        err, output, error = execute_helper.execute_command(
            [
                "cp",
                "user-sample-app-package_1.0_any.ipk",
                "{}/user-sample-app-package_1.0_any.ipk".format(
                    host_tutorials_dir
                ),
            ]
        )
        overall_result += err

        print("Creating {}/{}".format(host_tutorials_dir, USER_SAMPLE_TAR))
        err, output, error = execute_helper.execute_command(
            [
                "tar",
                "-cvf",
                "{}/{}".format(host_tutorials_dir, USER_SAMPLE_TAR),
                "user-sample-app-package_1.0_any.ipk",
                "payload_format_version",
            ]
        )
        overall_result += err

        # Create tar bundle with 5 good apps.
        print(
            "Creating {}/{}".format(host_tutorials_dir, MULTI_APP_ALL_GOOD_TAR)
        )
        err, output, error = execute_helper.execute_command(
            [
                "tar",
                "-cf",
                "{}/{}".format(host_tutorials_dir, MULTI_APP_ALL_GOOD_TAR),
                "sample-app-1-good_1.0_any.ipk",
                "sample-app-2-good_1.0_any.ipk",
                "sample-app-3-good_1.0_any.ipk",
                "sample-app-4-good_1.0_any.ipk",
                "sample-app-5-good_1.0_any.ipk",
                "payload_format_version",
            ]
        )
        overall_result += err

        # Create tar bundle with 4 good apps and one that cannot run.
        print(
            "Creating {}/{}".format(
                host_tutorials_dir, MULTI_APP_ONE_FAIL_RUN_TAR
            )
        )

        err, output, error = execute_helper.execute_command(
            [
                "tar",
                "-cf",
                "{}/{}".format(host_tutorials_dir, MULTI_APP_ONE_FAIL_RUN_TAR),
                "sample-app-1-good_1.0_any.ipk",
                "sample-app-2-good_1.0_any.ipk",
                "sample-app-3-good_1.0_any.ipk",
                "{}_1.1_any.ipk".format(BAD_RUNTIME_IMAGE),
                "sample-app-5-good_1.0_any.ipk",
                "payload_format_version",
            ]
        )
        overall_result += err

        # Create tar bundle with 4 good apps and one that cannot install.
        print(
            "Creating {}/{}".format(
                host_tutorials_dir, MULTI_APP_ONE_FAIL_INSTALL_TAR
            )
        )
        err, output, error = execute_helper.execute_command(
            [
                "tar",
                "-cf",
                "{}/{}".format(
                    host_tutorials_dir, MULTI_APP_ONE_FAIL_INSTALL_TAR
                ),
                "sample-app-1-good_1.0_any.ipk",
                "sample-app-2-good_1.0_any.ipk",
                "{}_1.1_invalid-architecture.ipk".format(
                    BAD_ARCHITECTURE_IMAGE
                ),
                "sample-app-4-good_1.0_any.ipk",
                "sample-app-5-good_1.0_any.ipk",
                "payload_format_version",
            ]
        )
        overall_result += err

        assert overall_result == 0
