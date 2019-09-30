#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest configuration file."""

import os
import pytest
import re
import subprocess
import sys
import tempfile
import urllib.request


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
    parser.addoption("--debug-output", action="store_true")
    parser.addoption("--device", action="store", default="none")
    parser.addoption("--dut", action="store", default="auto")
    parser.addoption(
        "--dut-download-dir", action="store", default="/tmp/pytest"
    )
    parser.addoption("--dut-tutorials-dir", action="store", default="/scratch")
    parser.addoption(
        "--host-download-dir", action="store", default="/tmp/pytest"
    )
    parser.addoption(
        "--host-tutorials-dir", action="store", default="/tmp/tutorials"
    )
    parser.addoption("--local-conf-file", action="store", default="")
    parser.addoption("--payload-version", action="store", default="1")
    parser.addoption("--venv", action="store", default="/tmp/venv")


def _download_from_url(url):

    filename = None

    if url != "":

        filename = os.path.join(tempfile.mkdtemp(), os.path.basename(url))

        try:
            urllib.request.urlretrieve(url, filename)
        except Exception as inst:
            print("\n\nError {}\n\n".format(type(inst)))
            print(
                "urlretrieve failed. Trying alternative method by adding "
                "mapping between 192.168.130.43 and "
                "artifactory-proxy.mbed-linux.arm.com into /etc/hosts."
            )

            try:
                f = open("/etc/hosts", "a")
                f.write(
                    "192.168.130.43  artifactory-proxy.mbed-linux.arm.com\n"
                )
            except Exception as inst:
                print("\n\nError {}\n\n".format(type(inst)))
                print("Could not update /etc/hosts. Perhaps run as root?")

            # re-try the urlretrieve.
            try:
                urllib.request.urlretrieve(url, filename)
            except Exception as inst:
                print("\n\nError {}\n\n".format(type(inst)))
                print("\n\nAlternative method also failed.\n\n")
                filename = None

    return filename


@pytest.fixture
def debug_output(request):
    """Fixture for --debug-output."""
    return request.config.getoption("--debug-output")


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
    """Fixture for --dut-download-dir."""
    return request.config.getoption("--dut-download-dir")


@pytest.fixture
def dut_tutorials_dir(request):
    """Fixture for --dut-tutorials-dir."""
    return request.config.getoption("--dut-tutorials-dir")


@pytest.fixture
def host_download_dir(request):
    """Fixture for --host-download-dir."""
    return request.config.getoption("--host-download-dir")


@pytest.fixture
def host_tutorials_dir(request):
    """Fixture for --host-tutorials-dir."""
    return request.config.getoption("--host-tutorials-dir")


@pytest.fixture
def local_conf_file(request):
    """
    Fixture for --local-conf-file.

    If the provided option starts with http, atttempt to download the file
    from the url and return the path.
    If the provided option is a file that exists then return that path.
    Else return None
    """
    if request.config.getoption("--local-conf-file").startswith("http"):
        return _download_from_url(
            request.config.getoption("--local-conf-file")
        )
    elif os.path.exists(request.config.getoption("--local-conf-file")):
        return request.config.getoption("--local-conf-file")
    else:
        return None


@pytest.fixture
def payload_version(request):
    """Fixture for --payload-version."""
    return request.config.getoption("--payload-version")


@pytest.fixture
def venv(request):
    """Fixture for --venv."""
    return request.config.getoption("--venv")


class ExecuteHelper:
    """Class to provide a wrapper for executing commands as a subprocess."""

    debug = False

    def __init__(self, request):
        """Initialise the class."""
        ExecuteHelper.debug = request.config.getoption("--debug-output")

    @staticmethod
    def _print(data):
        # method provided for debug support. Ordinarily this does nothing.
        # Uncomment the following line to get debug output.
        if ExecuteHelper.debug:
            print(data)

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
        """Execute the provided mbl-cli command.

        Executes the mbl-cli command and returns the error code, stdout and
        stderr.
        """
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
def execute_helper(request):
    """Fixture to return the helper class."""
    return ExecuteHelper(request)


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
