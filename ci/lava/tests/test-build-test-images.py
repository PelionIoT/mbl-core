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


class Test_Build_Test_Images:
    """Class to encalpulate building images for testing."""

    def _print_result(self, teststep, err):
        """Private function to provide LAVA formatted sub-results."""
        if err == 0:
            print(
                "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID={}-{} RESULT=pass>".format(
                    self.__class__.__name__, teststep
                )
            )
        else:
            print(
                "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID={}-{} RESULT=fail>".format(
                    self.__class__.__name__, teststep
                )
            )
        return err

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

    def test_build(self, device, execute_helper):
        """
        Build all of the images and tarballs.

        This function builds the the dockcross compiler and the test images
        needed for mbl-core tests and the HelloWorld update tests.
        """
        err = 0
        overall_err = 0

        if (
            device == "bcm2837-rpi-3-b-32"
            or device == "bcm2837-rpi-3-b-plus-32"
            or device == "imx7s-warp-mbl"
            or device == "imx7d-pico-mbl"
            or device == "imx6ul-pico-mbl"
        ):
            arm_arch = "armv7"
        elif device == "imx8mmevk-mbl":
            arm_arch = "arm64"
        else:
            print("Unsupported device type {}".format(device))
            err = 1

        overall_err += self._print_result("select-device", err)

        # If a valid device isn't specifed then exit with an error.
        if err == 1:
            assert False
            return

        os.chdir("tutorials/helloworld/")

        # Build dockcross
        err, output, error = execute_helper._execute_command(
            [
                "docker",
                "build",
                "-t",
                "linux-" + arm_arch + ":latest",
                "./cc-env/" + arm_arch,
            ]
        )

        overall_err += self._print_result("build-dockcross", err)
        if err != 0:
            assert False
            return

        err, output, error = execute_helper._execute_command(
            ["docker", "run", "--rm", "linux-" + arm_arch]
        )
        overall_err += self._print_result("run-dockcross", err)

        if err != 0:
            assert False
            return
        else:
            print("Creating build file")
            build = open("build-{}".format(arm_arch), "w")
            build.write(output)
            build.close()
            os.chmod("build-{}".format(arm_arch), 0o777)
            print("Created build file")

            print("Create User Sample App")
            err, output, error = execute_helper._execute_command(
                ["./build-{}".format(arm_arch), "make", "release"]
            )
            overall_err += self._print_result("build-application", err)

            print("Create Good Sample Apps")
            for image in range(1, 6):
                self._replace_line_in_file(
                    "src_opkg/CONTROL/control",
                    "Package:",
                    "sample-app-{}-good".format(image),
                )

                err, output, error = execute_helper._execute_command(
                    ["./build-{}".format(arm_arch), "make", "release"]
                )
                overall_err += self._print_result(
                    "build-sample-app-{}-good".format(image), err
                )

            print("Create Bad Architecture App")
            self._replace_line_in_file(
                "src_opkg/CONTROL/control", "Package:", BAD_ARCHITECTURE_IMAGE
            )
            self._replace_line_in_file(
                "src_opkg/CONTROL/control", "Version:", "1.1"
            )
            self._replace_line_in_file(
                "src_opkg/CONTROL/control",
                "Architecture:",
                "invalid-architecture",
            )

            err, output, error = execute_helper._execute_command(
                ["./build-{}".format(arm_arch), "make", "release"]
            )
            overall_err += self._print_result(
                "build-{}".format(BAD_ARCHITECTURE_IMAGE), err
            )

            print("Create Bad OCI Runtime App")
            self._replace_line_in_file(
                "src_opkg/CONTROL/control", "Package:", BAD_RUNTIME_IMAGE
            )
            self._replace_line_in_file(
                "src_opkg/CONTROL/control", "Architecture:", "any"
            )
            self._remove_first_line_from_file("src_bundle/config.json")

            err, output, error = execute_helper._execute_command(
                ["./build-{}".format(arm_arch), "make", "release"]
            )
            overall_err += self._print_result(
                "build-{}".format(BAD_RUNTIME_IMAGE), err
            )

            # Navigate to the location of the IPK
            os.chdir("release/ipk/")

            # Create tar bundles
            # User Sample App is needed in both ipk and tar format
            err, output, error = execute_helper._execute_command(
                [
                    "cp",
                    "user-sample-app-package_1.0_any.ipk",
                    "/tmp/user-sample-app-package_1.0_any.ipk",
                ]
            )
            overall_err += self._print_result(
                "copy-user-sample-app-package_1.0_any.ipk", err
            )

            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cvf",
                    "/tmp/user-sample-app-package_1.0_any.ipk.tar",
                    "user-sample-app-package_1.0_any.ipk",
                ]
            )
            overall_err += self._print_result(
                "tar-user-sample-app-package_1.0_any.ipk.tar", err
            )

            # Create tar bundle with 5 good apps.
            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cf",
                    "/tmp/mbl-multi-apps-update-package-all-good.tar",
                    "sample-app-1-good_1.0_any.ipk",
                    "sample-app-2-good_1.0_any.ipk",
                    "sample-app-3-good_1.0_any.ipk",
                    "sample-app-4-good_1.0_any.ipk",
                    "sample-app-5-good_1.0_any.ipk",
                ]
            )
            overall_err += self._print_result(
                "tar-mbl-multi-apps-update-package-all-good.tar", err
            )

            # Create tar bundle with 4 good apps and one that cannot run.
            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cf",
                    "/tmp/mbl-multi-apps-update-package-one-fail-run.tar",
                    "sample-app-1-good_1.0_any.ipk",
                    "sample-app-2-good_1.0_any.ipk",
                    "sample-app-3-good_1.0_any.ipk",
                    "{}_1.1_any.ipk".format(BAD_RUNTIME_IMAGE),
                    "sample-app-5-good_1.0_any.ipk",
                ]
            )
            overall_err += self._print_result(
                "tar-mbl-multi-apps-update-package-one-fail-run.tar", err
            )

            # Create tar bundle with 4 good apps and one that cannot install.
            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cf",
                    "/tmp/mbl-multi-apps-update-package-one-fail-install.tar",
                    "sample-app-1-good_1.0_any.ipk",
                    "sample-app-2-good_1.0_any.ipk",
                    "{}_1.1_invalid-architecture.ipk".format(
                        BAD_ARCHITECTURE_IMAGE
                    ),
                    "sample-app-4-good_1.0_any.ipk",
                    "sample-app-5-good_1.0_any.ipk",
                ]
            )
            overall_err += self._print_result(
                "tar-mbl-multi-apps-update-package-one-fail-install.tar", err
            )

            if overall_err != 0:
                assert False
            else:
                assert True
