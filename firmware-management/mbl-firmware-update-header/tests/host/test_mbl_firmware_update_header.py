#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import os
import pytest
import io

import mbl.firmware_update_header as uh

GOOD_TEST_CASES = [
    {
        "description": "Valid header blob without firmware signature",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x74\xc0\x49\xf4"
        ),
        "firmware_version": 1546446988,
        "firmware_size": 34203368,
        "firmware_hash": (
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
        ),
        "campaign_id": (
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
        ),
        "firmware_signature": b"",
    },
    {
        "description": "Valid header blob with firmware signature",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x08\x7a\x1b\xc1\xc6"
            b"\x01\x23\x45\x67\x89\xab\xcd\xef"
        ),
        "firmware_version": 1546446988,
        "firmware_size": 34203368,
        "firmware_hash": (
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
        ),
        "campaign_id": (
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
        ),
        "firmware_signature": b"\x01\x23\x45\x67\x89\xab\xcd\xef",
    },
]

BAD_TEST_CASES = [
    {
        "description": "Header blob with missing last (CRC) byte",
        "error_message_pattern": r"too short",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x74\xc0\x49"
        ),
    },
    {
        "description": "Header blob with invalid CRC",
        "error_message_pattern": r"CRC",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x74\xc0\x49\xf5"
        ),
    },
    {
        "description": "Header blob with invalid magic",
        "error_message_pattern": r"magic",
        "data": (
            b"\x5a\x51\xb3\xd5\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\xaa\x77\x69\xf1"
        ),
    },
    {
        "description": "Header blob with invalid format version",
        "error_message_pattern": r"version",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x01"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x3c\xec\xdf\xf4"
        ),
    },
    {
        "description": "Header blob with invalid "
        "firmware signature size/missing signature data",
        "error_message_pattern": r"too short",
        "data": (
            b"\x5a\x51\xb3\xd4\x00\x00\x00\x02"
            b"\x00\x00\x00\x00\x5c\x2c\xe8\x8c"
            b"\x00\x00\x00\x00\x02\x09\xe6\xe8"
            b"\x8e\x75\x5d\xef\x43\x4e\xa5\x94"
            b"\x3b\xeb\xd8\xfb\x0b\xa7\x69\xd4"
            b"\xcd\x01\xc9\x42\x21\xdb\x8e\xea"
            b"\x3b\xf7\x4f\x48\xeb\xbf\x41\xfb"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x08\x7a\x1b\xc1\xc6"
            b"\x01\x23\x45\x67\x89\xab\xcd"
        ),
    },
]


def _get_test_case_description(test_case):
    return test_case["description"]


@pytest.mark.parametrize(
    "test_case", GOOD_TEST_CASES, ids=_get_test_case_description
)
def test_unpack_valid_header(test_case):
    header = uh.FirmwareUpdateHeader()
    header.unpack(test_case["data"])

    assert header.firmware_version == test_case["firmware_version"]
    assert header.firmware_size == test_case["firmware_size"]
    assert header.firmware_hash == test_case["firmware_hash"]
    assert header.campaign_id == test_case["campaign_id"]
    assert header.firmware_signature == test_case["firmware_signature"]


@pytest.mark.parametrize(
    "test_case", BAD_TEST_CASES, ids=_get_test_case_description
)
def test_unpack_invalid_header(test_case):

    header = uh.FirmwareUpdateHeader()
    with pytest.raises(
        uh.FormatError, match=test_case["error_message_pattern"]
    ):
        header.unpack(test_case["data"])


@pytest.mark.parametrize(
    "test_case", GOOD_TEST_CASES, ids=_get_test_case_description
)
def test_pack_header(test_case):
    header = uh.FirmwareUpdateHeader()
    header.firmware_version = test_case["firmware_version"]
    header.firmware_size = test_case["firmware_size"]
    header.firmware_hash = test_case["firmware_hash"]
    header.campaign_id = test_case["campaign_id"]
    header.firmware_signature = test_case["firmware_signature"]

    data = header.pack()
    assert data == test_case["data"]


def test_calculate_firmware_hash():
    firmware_stream = io.BytesIO(b"test data")
    expected_hash = (
        b"\x91\x6f\x00\x27\xa5\x75\x07\x4c"
        b"\xe7\x2a\x33\x17\x77\xc3\x47\x8d"
        b"\x65\x13\xf7\x86\xa5\x91\xbd\x89"
        b"\x2d\xa1\xa5\x77\xbf\x23\x35\xf9"
    )
    assert uh.calculate_firmware_hash(firmware_stream) == expected_hash
