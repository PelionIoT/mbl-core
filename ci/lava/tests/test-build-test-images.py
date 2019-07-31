#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to build images to be used during testing.

Builds:
    HelloWorld

"""
import fileinput
import os
import pytest
import re
import subprocess


class Test_Build_Test_Images:
    def _print_result(self, teststep, err):
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

    def _replace_line_in_file(self, filename, search, replace):
        for line in fileinput.FileInput(filename, inplace=1):
            sline = line.strip().split(" ")
            if sline[0].startswith(search):
                sline[1] = replace
            line = " ".join(sline)
            print(line)

    def _remove_first_line_from_file(self, filename):
        with open(filename, "r") as fin:
            data = fin.read().splitlines(True)
        with open(filename, "w") as fout:
            fout.writelines(data[1:])

    def test_setup(self, device, execute_helper):

        err = 0

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

        self._print_result("select-device", err)

        if err == 1:
            assert False
            return

        os.chdir("tutorials/helloworld/")

        err, output, error = execute_helper._execute_command(
            [
                "docker",
                "build",
                "-t",
                "linux-" + arm_arch + ":latest",
                "./cc-env/" + arm_arch,
            ]
        )

        self._print_result("build-dockcross", err)
        if err != 0:
            assert False
            return

        err, output, error = execute_helper._execute_command(
            ["docker", "run", "--rm", "linux-" + arm_arch]
        )
        self._print_result("run-dockcross", err)

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
            self._print_result("build-application", err)

            for image in range(1, 6):
                self._replace_line_in_file(
                    "src_opkg/CONTROL/control",
                    "Package:",
                    "sample-app-{}-good".format(image),
                )

                err, output, error = execute_helper._execute_command(
                    ["./build-{}".format(arm_arch), "make", "release"]
                )

            self._replace_line_in_file(
                "src_opkg/CONTROL/control",
                "Package:",
                "sample-app-3-bad-architecture".format(image),
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

            self._replace_line_in_file(
                "src_opkg/CONTROL/control",
                "Package:",
                "sample-app-4-bad-oci-runtime".format(image),
            )
            self._replace_line_in_file(
                "src_opkg/CONTROL/control", "Architecture:", "any"
            )
            self._remove_first_line_from_file("src_bundle/config.json")

            err, output, error = execute_helper._execute_command(
                ["./build-{}".format(arm_arch), "make", "release"]
            )

            # Navigate to the location of the IPK
            os.chdir("release/ipk/")

            # Create tar bundles
            err, output, error = execute_helper._execute_command(
                [
                    "cp",
                    "user-sample-app-package_1.0_any.ipk",
                    "/tmp/user-sample-app-package_1.0_any.ipk",
                ]
            )
            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cvf",
                    "/tmp/user-sample-app-package_1.0_any.ipk.tar",
                    "user-sample-app-package_1.0_any.ipk",
                ]
            )

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

            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cf",
                    "/tmp/mbl-multi-apps-update-package-one-fail-run.tar",
                    "sample-app-1-good_1.0_any.ipk",
                    "sample-app-2-good_1.0_any.ipk",
                    "sample-app-3-good_1.0_any.ipk",
                    "sample-app-4-bad-oci-runtime_1.1_any.ipk",
                    "sample-app-5-good_1.0_any.ipk",
                ]
            )

            err, output, error = execute_helper._execute_command(
                [
                    "tar",
                    "-cf",
                    "/tmp/mbl-multi-apps-update-package-one-fail-install.tar",
                    "sample-app-1-good_1.0_any.ipk",
                    "sample-app-2-good_1.0_any.ipk",
                    "sample-app-3-bad-architecture_1.1_invalid-architecture.ipk",
                    "sample-app-4-good_1.0_any.ipk",
                    "sample-app-5-good_1.0_any.ipk",
                ]
            )

            self._print_result("create-tarfile", err)
            if err != 0:
                assert False
                return
            else:

                assert True
