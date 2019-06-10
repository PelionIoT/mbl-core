#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing the mbed-crypto library.

The actual tests should already be on the target, this script just runs them.
"""

import os
import logging
import pathlib
import pytest
import subprocess

TESTS_PATH = pathlib.Path("/usr/lib/mbed-crypto/test")

# For each test binary there is a corresponding ".datax" file. Enumerate
# the datax files then remove the ".datax" suffix to get a list of tests.
# convert the Paths to strings because that's what pytest.mark.parametrize
# expects for test IDs.
TESTS = [str(path.stem) for path in TESTS_PATH.glob("*.datax")]


def test_mbed_crypto_tests_exist():
    """Check that the test binaries are on the target"""
    assert TESTS


@pytest.mark.parametrize("test", TESTS, ids=TESTS)
def test_mbed_crypto(test):
    """Run a test binary on the target"""
    result = subprocess.run(
        [TESTS_PATH / test],
        check=False,
        cwd=TESTS_PATH,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        logging.getLogger().error(result.stdout)
    assert result.returncode == 0
