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
    parser.addoption("--device", action="store", default="none")
    parser.addoption("--dut", action="store", default="auto")


@pytest.fixture
def device(request):
    """Fixture for --device."""
    return request.config.getoption("--device")


@pytest.fixture
def dut(request):
    """Fixture for --dut."""
    return request.config.getoption("--dut")


class Execute_Helper:
    """Class to provide a wrapper for executing commands as a subprocess."""

    @staticmethod
    def _execute_command(command):
        """Execute the provided command list.

        Executes the command and returns the erro code, stdout and stderr.
        """
        print("execute_command start:")
        print("command:")
        print(command)
        p = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=-1,
            universal_newlines=True,
        )
        output, error = p.communicate()
        print("output:")
        print(output)
        print("error:")
        print(error)
        print("returnCode:")
        print(p.returncode)
        print("execute_command done.")

        return p.returncode, output, error

    @staticmethod
    def _send_mbl_cli_command(command, addr):

        cliCommand = ["mbl-cli", "--address", addr]
        for item in command:
            cliCommand.append(item)

        return Execute_Helper._execute_command(cliCommand)


@pytest.fixture
def execute_helper():
    """Fixture to return the helper class."""
    return Execute_Helper


@pytest.fixture
def dut_addr(dut, execute_helper):
    """Fixture to calculate the DUT IP address."""
    dut_addr = ""

    returnCode, list_output, error = execute_helper._execute_command(
        ["mbl-cli", "list"]
    )

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
