#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Helpers modeule.

This module contains methods that are used across pytest classes.
"""

import os
import re
import subprocess
import tempfile
import urllib.request
from pathlib import Path


def compare_files(dut_addr, execute_helper, file1, file2):
    """Run cmp command on files passed as argument using mbl-cli."""
    cmp_command = "cmp -s {} {}".format(file1, file2)
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'su -l -c "{}"'.format(cmp_command)], dut_addr
    )
    return exit_code


def dd_bootloader_component(
    dut_addr, execute_helper, update_component_name, output_type, stage
):
    """Run dd command to read bootloader data from the disk using mbl-cli.

    This method is used to read data from the block device. Depending what the
    output type is, different portion of the disk are read.
    If the output_type is "partition" the whole partition of the bootloader
    component is read: we need the offset and size of the whole partion and
    this info is contained in /config/factory/part-info.
    If the output_type is "file" we need to read only the portion of the
    partition where the bootloader component has been previously written: we
    need the offset of the partition and the size of the bootloader component
    has been written into the disk.
    NOTE: the 2 dd commands have different "bs" order of magnitude. In case of
    the file this is 1 (1bytes) because we need read from the disk exact X
    bytes in order to compare with the output file with the bootloader
    component.
    """
    part_info = Path("/config/factory/part-info")
    output_file_path = Path("/scratch") / "{}_{}_{}".format(
        update_component_name, output_type, stage
    )
    align_file_path = part_info / "MBL_{}_OFFSET_BANK1_KiB".format(
        update_component_name.upper()
    )
    if output_type == "partition":
        partition_size_file_path = part_info / "MBL_{}_SIZE_KiB".format(
            update_component_name.upper()
        )
        count_command = "cat {}".format(partition_size_file_path)
        size_magnitude = "K"
    elif output_type == "file":
        component_file_path = Path("/scratch") / update_component_name.upper()
        count_command = "stat -c%s {}".format(component_file_path)
        size_magnitude = ""
    dd_command = (
        "set -x; "
        "BD=$(/sbin/blkid -L rootfs1 | sed 's/p[0-9]+$//'); "
        "OF={}; "
        "SKIP=$(cat {})K; "
        "COUNT=$({}); "
        "dd if=$BD of=$OF skip=$SKIP count=$COUNT bs=1{} "
        "iflag=skip_bytes".format(
            output_file_path, align_file_path, count_command, size_magnitude
        )
    )
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", dd_command], dut_addr
    )
    return exit_code, output_file_path


def download_from_url(url):
    """Download a file from the url.

    This method is very specific to our LAVA insfrastructure.
    If it fails to download the file, it tries to use the proxy set up in Oulu.
    """
    filename = None
    if url:
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


def get_dut_address(dut, execute_helper):
    """Calculate the DUT IP address using mbl-cli."""
    dut_addr = ""

    exit_code, output, error = execute_helper.execute_command(
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
        for line in output.splitlines():
            match = pattern.match(line)
            if match:
                dut_addr = match.group("address")
                break
    else:
        # Match a device against the provided dut parameter
        pattern = re.compile(r"\s*.:[^:]+:\s+(?P<address>\S+)")
        for line in output.splitlines():
            if dut in line:
                match = pattern.match(line)
                if match:
                    dut_addr = match.group("address")
                    break
    return dut_addr


def get_kernel_version(dut_addr, execute_helper):
    """Get the kernel version running on the DUT using mbl-cli."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'su -l -c "uname -a"'], dut_addr
    )
    return exit_code, output


def strings_grep(dut_addr, execute_helper, file_path, pattern):
    """Run strings command on a file and grep for a pattern using mbl-cli."""
    command = "strings {} | grep {}".format(file_path, pattern)
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'su -l -c "{}"'.format(command)], dut_addr
    )
    return exit_code, output, error


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
        exit_code = 1
        output = ""
        error = ""

        if addr != "":
            cli_command = ["mbl-cli", "--address", addr]
            for item in command:
                cli_command.append(item)
            exit_code, output, error = ExecuteHelper.execute_command(
                cli_command
            )
        return exit_code, output, error
