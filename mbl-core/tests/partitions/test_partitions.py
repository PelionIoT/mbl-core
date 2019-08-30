#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Pytest for checking partition offsets and sizes.
"""

import json
import pathlib
import pytest
import re
import subprocess

PART_INFO_DIR = pathlib.Path("/config/factory/part-info")


def get_int_var_for_part(var_name, part_name, default=None):
    """
    Get the value of a partition config variable for a partition.

    Args:
    * var_name (str): name of partition property to get.
    * part_name (str): name of partition.
    * default: if not None, a value to return when the config file doesn't
      exist.

    Returns (str): the contents of the config file for the given variable name
    and partition.
    """
    filename = "MBL_{}_{}".format(re.escape(part_name), re.escape(var_name))
    path = PART_INFO_DIR / filename
    if default is not None and not path.is_file():
        return default
    return int(path.read_text())


def get_part_names():
    """Get the names of all partitions, including non-file system partitions."""
    filename_re = re.compile(r"^MBL_([A-Z0-9_]+)_SIZE_KiB$")
    part_names = []
    for f in PART_INFO_DIR.iterdir():
        filename_match = filename_re.match(f.name)
        if filename_match:
            part_name = filename_match.group(1)
            if get_int_var_for_part("SKIP", part_name, 0):
                continue
            part_names.append(part_name)
    return part_names


def is_fs_partition(part_name):
    """Return true if the named partition has a file system."""
    return not get_int_var_for_part("NO_FS", part_name, 0)


def get_fs_part_names():
    """Get the names of all file system partitions"""
    return filter(is_fs_partition, get_part_names())


def get_expected_part_table():
    """
    Get the expected partition table.

    Returns a list of (offset, size) pairs for file system partitions, based on
    the config files left in the factory config partition.

    The list is sorted and the offsets and sizes are in KiB units.
    """
    part_table = []
    for part_name in get_fs_part_names():
        size_KiB = get_int_var_for_part("SIZE_KiB", part_name)
        offset1_KiB = get_int_var_for_part("OFFSET_BANK1_KiB", part_name)
        part_table.append((offset1_KiB, size_KiB))
        if get_int_var_for_part("IS_BANKED", part_name, 0):
            offset2_KiB = get_int_var_for_part("OFFSET_BANK2_KiB", part_name)
            part_table.append((offset2_KiB, size_KiB))

    return sorted(part_table)


def get_device_file_for_parts():
    """
    Get the path to the device file that contains the file system partitions.

    This function assumes that all of the partitions are on the same device.
    """
    return subprocess.check_output(["blkid", "-L", "rootfs1"]).rsplit(b"p", 1)[
        0
    ]


def get_actual_part_table():
    """
    Get the actual partition table.

    Returns a list of (offset, size) pairs for file system partitions, based on
    the output of "sfdisk -J".

    The list is sorted and the offsets and sizes are in KiB units.
    """
    device_file = get_device_file_for_parts()
    # "sfdisk -J" gives JSON output
    sfdisk_json = subprocess.check_output(["sfdisk", "-J", device_file])

    # The output contains some info we're not interested in - we just want the
    # "partitions" list which contains dicts with an offsets and sizes for each
    # partition.
    sfdisk_part_table = json.loads(sfdisk_json)["partitiontable"]["partitions"]

    # Convert the partition table from sfdisk into a list of (offset, size)
    # tuples. Divide the numbers by 2 to convert 512B sectors that sfdisk uses
    # into KiB
    part_table = sorted(
        [
            (int(d["start"]) // 2, int(d["size"]) // 2)
            for d in sfdisk_part_table
        ]
    )

    # The fourth partition is an "extended partition" that just contains other
    # partitions. Ignore it.
    del part_table[3]

    return part_table


def is_part_var_name(var_name):
    """
    Return true if the named variable is one of the MBL partition variables
    we're interested in.
    """
    return var_name.startswith("MBL_") and any(
        name in var_name for name in get_part_names()
    )


def test_part_table():
    """
    Test that the partition table on the system matches the expected partition table.

    The idea is to create two lists of (offset, size) pairs:
    * one based on partition data left in the factory config partition;
    * one based on the output of "sfdisk -J".

    We then check that the two lists are equal.
    """
    assert get_actual_part_table() == get_expected_part_table()


@pytest.mark.parametrize("part_name", get_fs_part_names())
def test_size_KiB_consistent_with_size_MiB(part_name):
    """
    Test that the size in KiB recorded for a partition is always 1024 times the
    size in MiB recorded for that partition
    """
    size_KiB = get_int_var_for_part("SIZE_KiB", part_name)
    size_MiB = get_int_var_for_part("SIZE_MiB", part_name)
    assert size_KiB == 1024 * size_MiB


@pytest.mark.parametrize("part_name", get_part_names())
def test_part_offset_is_aligned(part_name):
    """
    Test that the offset of the named partition is actually aligned according
    to the alignment value we recorded for the partition.
    """
    align_KiB = get_int_var_for_part("ALIGN_KiB", part_name)
    offset1_KiB = get_int_var_for_part("OFFSET_BANK1_KiB", part_name)
    assert offset1_KiB % align_KiB == 0
    if get_int_var_for_part("IS_BANKED", part_name, 0):
        offset2_KiB = get_int_var_for_part("OFFSET_BANK2_KiB", part_name)
        assert offset2_KiB % align_KiB == 0


def test_local_conf_part_setting_has_taken_effect(local_conf_assignment):
    """
    If a partition property was set in local.conf, check that the property
    matches what was set in the partition info files.
    """
    var_name, var_value = local_conf_assignment
    if is_part_var_name(var_name):
        var_path = PART_INFO_DIR / var_name
        assert var_path.is_file()
        assert var_path.read_text() == var_value


@pytest.mark.parametrize("part_name", get_fs_part_names())
def test_local_conf_flash_erase_block_size_setting_has_taken_effect(
    local_conf_assignments_dict, part_name
):
    """
    If the flash erase block size was set in local.conf, make sure all file
    system partition alignments match that block size... Unless the partition's
    alignment itself was overriden in local.conf.
    """
    flash_erase_block_size_KiB = int(
        local_conf_assignments_dict.get("MBL_FLASH_ERASE_BLOCK_SIZE_KiB", 0)
    )
    if (
        flash_erase_block_size_KiB
        and "MBL_{}_ALIGN_KiB".format(part_name)
        not in local_conf_assignments_dict
    ):
        assert (
            get_int_var_for_part("ALIGN_KiB", part_name)
            == flash_erase_block_size_KiB
        )
