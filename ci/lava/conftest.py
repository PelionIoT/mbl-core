#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest configuration file."""

import pytest
import re
import subprocess
import sys


def pytest_report_teststatus(report):
    """Override Pytest hook to report test status."""
    if report.when == "call":
        if report.passed:
            lava = "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID={} RESULT=pass>".format(
                report.nodeid.replace(" ", "_")
            )
            return report.outcome, "*", lava

    if report.failed:
        lava = "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID={} RESULT=fail>".format(
            report.nodeid.replace(" ", "_")
        )
        return report.outcome, "*", lava


def pytest_addoption(parser):
    """Add option parsing to pytest."""
    parser.addoption("--debug_output", action="store_true")
    parser.addoption("--device", action="store", default="none")
    parser.addoption("--dut", action="store", default="auto")
    parser.addoption(
        "--dut_download_dir", action="store", default="/tmp/pytest"
    )
    parser.addoption("--dut_tutorials_dir", action="store", default="/scratch")
    parser.addoption(
        "--host_download_dir", action="store", default="/tmp/pytest"
    )
    parser.addoption(
        "--host_tutorials_dir", action="store", default="/tmp/tutorials"
    )
    parser.addoption("--venv", action="store", default="/tmp/venv")


@pytest.fixture
def debug_output(request):
    """Fixture for --debug_output."""
    return request.config.getoption("--debug_output")


@pytest.fixture
def device(request):
    """Fixture for --device."""
    return request.config.getoption("--device")


@pytest.fixture
def dut(request):
    """Fixture for --dut."""
    return request.config.getoption("--dut")


@pytest.fixture
def dut_download_dir(request):
    """Fixture for --dut_download_dir."""
    return request.config.getoption("--dut_download_dir")


@pytest.fixture
def dut_tutorials_dir(request):
    """Fixture for --dut_tutorials_dir."""
    return request.config.getoption("--dut_tutorials_dir")


@pytest.fixture
def host_download_dir(request):
    """Fixture for --host_download_dir."""
    return request.config.getoption("--host_download_dir")


@pytest.fixture
def host_tutorials_dir(request):
    """Fixture for --host_tutorials_dir."""
    return request.config.getoption("--host_tutorials_dir")


@pytest.fixture
def venv(request):
    """Fixture for --venv."""
    return request.config.getoption("--venv")


class ExecuteHelper:
    """Class to provide a wrapper for executing commands as a subprocess."""

    @staticmethod
    def _print(data):
        # method provided for debug support. Ordinarily this does nothing.
        # Uncomment the following line to get debug output.
        #print(data)
        pass

    @staticmethod
    def execute_command(command):
        """Execute the provided command list.

        Executes the command and returns the error code, stdout and stderr.
        """
        ExecuteHelper._print("execute_command start:")
        ExecuteHelper._print("command:")
        ExecuteHelper._print(command)
        p = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=-1,
            universal_newlines=True,
        )
        output, error = p.communicate()
        ExecuteHelper._print("output:")
        ExecuteHelper._print(output)
        ExecuteHelper._print("error:")
        ExecuteHelper._print(error)
        ExecuteHelper._print("returnCode:")
        ExecuteHelper._print(p.returncode)
        ExecuteHelper._print("execute_command done.")

        return p.returncode, output, error

    @staticmethod
    def send_mbl_cli_command(command, addr):

        err = 1
        output = ""
        error = ""

        if addr != "":
            cli_command = ["mbl-cli", "--address", addr]
            for item in command:
                cli_command.append(item)
            err, output, error = ExecuteHelper.execute_command(cli_command)
        return err, output, error


@pytest.fixture
def execute_helper():
    """Fixture to return the helper class."""
    return ExecuteHelper


@pytest.fixture
def dut_addr(dut, execute_helper):
    """Fixture to calculate the DUT IP address."""
    dut_addr = ""

    return_code, list_output, error = execute_helper.execute_command(
        ["mbl-cli", "list"]
    )

    # The default value for dut is set to auto. This means use the first
    # device returned by the mbl-cli. In the test farm set up there is a
    # one-to-one mapping of device to container, so this is sufficient.
    # However, allowing a dut to be specified allows manual testing when
    # there is more than one device detectable on the network.
    #
    # The mbl-cli list command returns data in the following format:
    #     Discovering devices. This will take up to 30 seconds.
    #     1: mbed-linux-os-2075: fe80::21f:7bff:fe86:a738%enp0s31f6
    #     2: mbed-linux-os-158: fe80::ecfb:3fff:fece:9376%enp0s20f0u14u1
    #
    # The regular expressions below extract the IP address and interface
    # name as that is used to communicate to the DUT.

    if dut == "auto":
        # Take the first found device and use that
        pattern = re.compile(r"\s*1:[^:]+:\s+(?P<address>\S+)")
        for line in list_output.splitlines():
            match = pattern.match(line)
            if match:
                dut_addr = match.group("address")
                break
    else:
        # Match a device against the provided dut parameter
        pattern = re.compile(r"\s*.:[^:]+:\s+(?P<address>\S+)")
        for line in list_output.splitlines():
            if dut in line:
                match = pattern.match(line)
                if match:
                    dut_addr = match.group("address")
                    break

    return dut_addr
