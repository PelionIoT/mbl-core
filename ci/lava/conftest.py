#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest configuration file."""

import os
import pytest
import re

from helpers import download_from_url, get_dut_address, ExecuteHelper


def pytest_addoption(parser):
    """Add option parsing to pytest."""
    # Common parameters
    parser.addoption("--venv", action="store", default="/tmp/venv")
    parser.addoption("--debug-output", action="store_true")
    parser.addoption("--device", action="store", default="none")
    parser.addoption("--dut", action="store", default="auto")
    parser.addoption(
        "--dut-download-dir", action="store", default="/tmp/pytest"
    )
    parser.addoption(
        "--host-download-dir", action="store", default="/tmp/pytest"
    )

    # Tutorial tests parameters
    parser.addoption("--dut-artifacts-dir", action="store", default="/scratch")
    parser.addoption(
        "--host-artifacts-dir", action="store", default="/tmp/tutorials"
    )
    parser.addoption("--payload-version", action="store", default="1")

    # local.conf tests parameters
    parser.addoption("--local-conf-file", action="store", default="")

    # Component update test parameters
    parser.addoption("--payloads-url", action="store")
    parser.addoption(
        "--update-component-name",
        action="store",
        choices=[
            "sample-app",
            "bootloader1",
            "bootloader2",
            "kernel",
            "multi-app-all-good",
            "multi-component",
            "rootfs",
        ],
    )
    parser.addoption("--update-payload-url", action="store")
    parser.addoption("--update-payload-testinfo-url", action="store")
    parser.addoption(
        "--update-method",
        action="store",
        default="",
        choices=["pelion", "mbl-cli"],
    )


def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line(
        "markers",
        "pelion: mark a test to be run when the update method is pelion",
    )
    config.addinivalue_line(
        "markers",
        "mbl_cli: mark a test to be run when the update method is mbl-cli",
    )


def pytest_collection_modifyitems(config, items):
    """Select tests depending the update-method passed as argument."""
    update_method = config.getoption("--update-method")
    if not update_method:
        return
    skip = pytest.mark.skip(
        reason="update-method {} selected".format(update_method)
    )
    for item in items:
        markers = set(m.name for m in item.iter_markers())
        # We skip tests which don't match the update_method
        # mbl-cli is replaced with mbl_cli
        if markers and update_method.replace("-", "_") not in markers:
            print(item)
            item.add_marker(skip)


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


def pytest_generate_tests(metafunc):
    """Select tests that have the update_component_name specified as marker."""
    # This is called for every test. Only get/set command line arguments
    # if the argument is specified in the list of test "fixturenames".
    option_value = metafunc.config.option.update_component_name
    if (
        "update_component_name" in metafunc.fixturenames
        and option_value is not None
    ):
        metafunc.parametrize("update_component_name", [option_value])


def get_local_conf_assignments_dict(local_conf_file):
    """
    Get a list of the variable (name, value) pairs for each simple assignment
    in the given local.conf.

    Note that this function only deals with values that are double quoted and
    doesn't deal with override syntax, variable appends, variable prepends or
    "weak" assignments.

    I.e. the variable values returned by this function are NOT guaranteed to be
    final values used by BitBake.

    Returns a dict mapping variable names to the values assigned to them.
    """
    assignment_re = re.compile(
        r"""^\s*
            (?P<var_name> [A-Za-z0-9_]+ )
            \s* = \s*
            "(?P<var_value> [^"]* )"
            \s*
        $""",
        re.X,
    )
    assignments = {}
    with open(local_conf_file, "r") as config:
        for line in config:
            m = assignment_re.match(line)
            if m:
                # Don't check if the var name is already in our dict - just let
                # later assignments take precedence.
                assignments[m.group('var_name')] = m.group('var_value')
    return assignments


@pytest.fixture
def local_conf_assignments_dict(local_conf_file):
    """
    Fixture for the dictionary mapping variable names to their values for
    assignments in local.conf.
    The caveats in the docstring for get_local_conf_assignments_dict apply here
    too.
    """
    if local_conf_file is None:
        return {}
    return get_local_conf_assignments_dict(local_conf_file)


# common fixtures
@pytest.fixture
def venv(request):
    """Fixture for --venv."""
    return request.config.getoption("--venv")


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
def host_download_dir(request):
    """Fixture for --host-download-dir."""
    return request.config.getoption("--host-download-dir")


# Tutorials fixtures
@pytest.fixture
def dut_artifacts_dir(request):
    """Fixture for --dut-artifacts-dir."""
    return request.config.getoption("--dut-artifacts-dir")


@pytest.fixture
def host_artifacts_dir(request):
    """Fixture for --host-artifacts-dir."""
    return request.config.getoption("--host-artifacts-dir")


@pytest.fixture
def payload_version(request):
    """Fixture for --payload-version."""
    return request.config.getoption("--payload-version")


# local.conf fixtures
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
        return download_from_url(request.config.getoption("--local-conf-file"))
    elif os.path.exists(request.config.getoption("--local-conf-file")):
        return request.config.getoption("--local-conf-file")
    else:
        return None


# Component update fixtures
@pytest.fixture
def update_component_name(request):
    """Fixture for --update-component-name."""
    return request.config.getoption("--update-component-name")


@pytest.fixture
def update_payload(request):
    """Fixture for --update-payload-url."""
    return download_from_url(request.config.getoption("--update-payload-url"))


@pytest.fixture
def payloads_url(request):
    """Fixture for --payloads-url."""
    return request.config.getoption("--payloads-url")


@pytest.fixture
def update_payload_testinfo(request):
    """
    Fixture for --update-payload-testinfo-url.

    Returns the testinfo data as a dictionary
    """
    return download_from_url(
        request.config.getoption("--update-payload-testinfo-url")
    )


@pytest.fixture
def update_method(request):
    """Fixture for --update-method."""
    return request.config.getoption("--update-method")


# DUT fixtures
@pytest.fixture
def execute_helper(request):
    """Fixture to return the helper class."""
    return ExecuteHelper(request)


@pytest.fixture
def dut_addr(dut, execute_helper):
    """Fixture to return the DUT address."""
    return get_dut_address(dut, execute_helper)
