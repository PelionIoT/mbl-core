#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing the psa_trusted_storage_linux library.

The actual tests should already be on the target, this script just runs them.
"""

import os
import logging
import pathlib
import pytest
import subprocess


TESTS_PATH = "/usr/bin/"
TESTS = ["psa-storage-example-app"]


def test_psa_trusted_storage_linux_tests_exist():
    """Check that the test binaries are on the target"""
    assert TESTS


@pytest.mark.parametrize("test", TESTS, ids=TESTS)
def test_psa_trusted_storage_linux(test):
    """Run a test binary on the target"""
    result = subprocess.run(
        [test], check=False, cwd=TESTS_PATH, capture_output=True, text=True
    )
    if result.returncode != 0:
        logging.getLogger().error(result.stdout)
    assert result.returncode == 0
