#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Pytest for checking the partitions are as we expect them (offsets, sizes,
devices, etc.)
"""
import glob
import os
import pathlib
import pytest
import re
import subprocess

EXPECTED_PART_INFO_DIR = pathlib.Path("/config/factory/part-info")
ACTUAL_PART_INFO_DIR = pathlib.Path("/sys/block")


def get_var_for_part(var_name, part_name, default=None):
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
    filename = "MBL_{}_{}".format(part_name, var_name)
    path = EXPECTED_PART_INFO_DIR / filename
    if default is not None and not path.is_file():
        return default
    return path.read_text()


def get_bool_var_for_part(var_name, part_name):
    """
    Get the value of a boolean partition config variable for a partition.

    Args:
    * var_name (str): name of partition property to get.
    * part_name (str): name of partition.

    Returns (bool): the contents of the config file for the given variable name
    and partition converted to a boolean, or False if the file doesn't exist.
    """
    # Convert to int so that e.g. " " doesn't count as true.
    # Convert to bool to get the return type right.
    return bool(int(get_var_for_part(var_name, part_name, "0")))


def is_part_skipped(part_name):
    """Return True if the given partition is skipped on the device."""
    return get_bool_var_for_part("SKIP", part_name)


def is_part_banked(part_name):
    """Return True if the given partition is banked."""
    return get_bool_var_for_part("IS_BANKED", part_name)


def is_fs_part(part_name):
    """Return True if the given partition is a file system partition."""
    return not get_bool_var_for_part("NO_FS", part_name)


def get_all_part_names():
    """
    Get the names of all partitions (including "raw" partitions).
    """
    part_names = []
    filename_re = re.compile(r"^MBL_([A-Z0-9_]+)_SIZE_KiB$")
    for f in EXPECTED_PART_INFO_DIR.iterdir():
        filename_match = filename_re.match(f.name)
        if filename_match:
            part_name = filename_match.group(1)
            if is_part_skipped(part_name):
                continue
            part_names.append(part_name)
    return part_names


def get_fs_part_names():
    """
    Get the names of all file system partitions.
    """
    return filter(is_fs_part, get_all_part_names())


def get_part_bank_numbers(part_name):
    """Get a tuple of bank numbers for the given partition."""
    if is_part_skipped(part_name):
        return []
    if is_part_banked(part_name):
        return [1, 2]
    return [1]


def get_expected_part_table():
    """
    Get the expected partition table.

    Returns a list of (offset, size) pairs for file system partitions, based on
    the config files left in the factory config partition.

    The list is sorted and the offsets and sizes are in KiB units.
    """
    part_table = []
    for part_name in get_fs_part_names():
        size_KiB = int(get_var_for_part("SIZE_KiB", part_name))
        for bank in get_part_bank_numbers(part_name):
            offset_KiB = int(
                get_var_for_part("OFFSET_BANK{}_KiB".format(bank), part_name)
            )
            part_table.append((offset_KiB, size_KiB))

    return sorted(part_table)


def get_block_device_for_parts():
    """
    Get the name of the block device that contains the file system partitions.

    The returned name does not include any partition numbers, just the name of
    the parent device. E.g. "mmcblk1".

    This function assumes that all of the partitions are on the same device.
    """

    lsblk_output = subprocess.check_output(
        ["lsblk", "-n", "-o", "MOUNTPOINT,PKNAME"], text=True
    )
    for line in lsblk_output.splitlines():
        fields = line.split()
        if len(fields) == 2 and fields[0] == "/":
            return fields[1]
    raise AssertionError("Failed to find rootfs entry in lsblk output")


def get_actual_part_table():
    """
    Get the actual partition table.

    Returns a list of (offset, size) pairs for file system partitions, based on
    the content of /sys/block/{block_device}/{partition}


    The list is sorted and the offsets and sizes are in KiB units.
    """
    block_device = get_block_device_for_parts()
    device_dir = ACTUAL_PART_INFO_DIR / block_device

    part_table = []

    for f in device_dir.glob("{}p[0-9]*".format(glob.escape(block_device))):
        size_in_sectors = int((f / "size").read_text())
        start_in_sectors = int((f / "start").read_text())
        # Divide the numbers by 2 to convert 512B sectors into KiB
        part_table.append((start_in_sectors // 2, size_in_sectors // 2))

    part_table = sorted(part_table)

    # The fourth partition is an "extended partition" that just contains other
    # partitions. Ignore it.
    del part_table[3]

    return part_table


def test_part_table():
    # Create two lists of (offset, size) pairs:
    # * one based on partition data left in the factory config partition,
    # * one based on the content of /sys/block,
    # then check that the two lists are equal.
    assert get_actual_part_table() == get_expected_part_table()


def test_part_device_name_files():
    device_name = get_block_device_for_parts()
    for part_name in get_all_part_names():
        for bank in get_part_bank_numbers(part_name):
            assert (
                get_var_for_part("DEVICE_NAME_BANK{}".format(bank), part_name)
                == device_name
            )


@pytest.mark.parametrize("part_name", get_fs_part_names())
def test_size_KiB_consistent_with_size_MiB(part_name):
    """
    Test that the size in KiB recorded for a partition is always 1024 times the
    size in MiB recorded for that partition
    """
    size_KiB = int(get_var_for_part("SIZE_KiB", part_name))
    size_MiB = int(get_var_for_part("SIZE_MiB", part_name))
    assert size_KiB == 1024 * size_MiB


def test_local_conf_part_setting_has_taken_effect(local_conf_assignments_dict):
    """
    If a partition property was set in local.conf, check that the property
    matches what was set in the partition info files.
    """
    local_conf_assignment = local_conf_assignments_dict.items()

    for local_conf_assignment in local_conf_assignments_dict.items():
        var_name, var_value = local_conf_assignment
        var_path = EXPECTED_PART_INFO_DIR / var_name
        if var_path.is_file():
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
            int(get_var_for_part("ALIGN_KiB", part_name))
            == flash_erase_block_size_KiB
        )
