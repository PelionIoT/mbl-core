#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Script to run psa-arch-tests.

The actual tests should already be on the target, this script just runs them
via mbl-cli and translates the output into something LAVA can understand.
"""
import re
import subprocess
import sys

SUITES = ["crypto"]

# Tests that we know will fail.
#
# These will be reported to Lava as skipped when they fail so that engineers
# don't waste time investigating the failures. If the tests actually pass then
# they will be reported to Lava as failures so that we notice and update this
# list.
EXPECTED_FAILS = {
    # Bug in mbed-crypto: https://github.com/ARMmbed/mbed-crypto/issues/126
    "239_Testing_crypto_asymmetric_APIs": True
}


def _get_id_from_line(line):
    match_pattern = re.compile(
        r"""
            ^ \s*
            TEST: \s* (?P<numeric_id>\d+)
            \s* \| \s*
            DESCRIPTION: \s* (?P<description>.*\S)
            \s* $
        """,
        flags=re.VERBOSE,
    )
    match = match_pattern.match(line)
    if not match:
        return None
    numeric_id, description = match.group("numeric_id", "description")
    subs_pattern = re.compile("[^A-Za-z0-9_]+")
    return "{}_{}".format(
        match.group("numeric_id"),
        subs_pattern.sub("_", match.group("description")),
    )


def _get_result_from_line(line):
    pattern = re.compile(r"^\s*TEST RESULT:\s*(?P<result>PASS|FAIL|SKIP)")
    match = pattern.match(line)
    if not match:
        return None
    return match.group("result")


def _check_expected_fails(current_id, result):
    """
    Modify result to trick LAVA about expected failures.

    This is a hack because LAVA doesn't support "expected fail" results. If an
    "expected fail" test fails, tell LAVA it was skipped so that it doesn't
    cause much fuss. If the test succeeds, lie to LAVA and say it failed. This
    should cause somebody to look into the failure and maybe remove it from the
    EXPECTED_FAILS list.
    """
    if current_id in EXPECTED_FAILS:
        if result == "fail":
            return "skip"
        else:
            return "fail"
    return result


def _report_result(current_id, result):
    if not current_id:
        current_id = "unknown"

    result = _check_expected_fails(current_id, result.lower())

    print(
        "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID={} RESULT={}>".format(
            current_id, result
        )
    )


def _get_dut_addr():
    list_output = subprocess.check_output(
        ["mbl-cli", "list"], universal_newlines=True
    )

    pattern = re.compile(r"\s*1:[^:]+:\s+(?P<address>\S+)")
    for line in list_output.splitlines():
        print(line)
        match = pattern.match(line)
        if match:
            return match.group("address")
    return None


def _run_suite(suite, dut_addr):
    with subprocess.Popen(
        [
            "mbl-cli",
            "--address",
            dut_addr,
            "shell",
            "psa-arch-{}-tests".format(suite),
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    ) as proc:
        current_id = None
        for line in proc.stdout:
            line = line.strip()
            print(line)
            test_id = _get_id_from_line(line)
            if test_id:
                current_id = test_id
                continue
            result = _get_result_from_line(line)
            if result:
                _report_result(current_id, result)
                current_id = None


dut_addr = _get_dut_addr()
if not dut_addr:
    _report_result("get_dut_addr", "FAIL")
    sys.exit(1)

for suite in SUITES:
    _run_suite(suite, dut_addr)

sys.exit(0)
