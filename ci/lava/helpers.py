#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""
Helpers module.

This module contains helper functions and classes that are used across pytest
modules.
"""

import os
import re
import subprocess
import sys
import tempfile
import urllib.request
from urllib.error import HTTPError
from pathlib import Path


def compare_files(dut_addr, execute_helper, file1, file2):
    """Run cmp command on files passed as argument using mbl-cli."""
    cmp_command = "cmp -s {} {}".format(file1, file2)
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'sh -l -c "{}"'.format(cmp_command)], dut_addr
    )
    return exit_code


def read_partition_to_file(
    dut_addr, execute_helper, partition_name, stage, component_size=None
):
    """Read partition data from the disk on the DUT to a file.

    This function assumes we have a /config/factory/part-info directory on the
    target, which contains a set of files holding information about the
    partition offsets and sizes.

    If the component_size is None, we default to reading the entire partition
    using the offset and size data from the part-info files.

    If component_size is given, we expect the order of magnitude is bytes.
    We get the offset from the part-info file and read component_size bytes
    from the disk.

    :param partition_name str: name of the partition, also used to name
        the extracted file.
    :param stage str: append a stage to the output file name (e.g 'pre' or
        'post')
    :param component_size int: (optional) number of bytes to read from disk.
    """
    part_info = Path("/config/factory/part-info")
    output_file_path = Path("/scratch") / "{}_{}".format(partition_name, stage)
    align_file_path = part_info / "MBL_{}_OFFSET_BANK1_KiB".format(
        partition_name.upper()
    )
    partition_size_file_path = part_info / "MBL_{}_SIZE_KiB".format(
        partition_name.upper()
    )
    _, partition_size, _ = execute_helper.send_mbl_cli_command(
        ["shell", "cat {}".format(partition_size_file_path)],
        dut_addr,
        raise_on_error=True,
    )
    partition_size = partition_size.splitlines()[1]
    if component_size is None:
        count = partition_size
        # set the magnitude to KiB as the size is given in KiB
        size_magnitude = "K"
    elif (component_size / 1024) <= int(partition_size):
        count = component_size
        # set the magnitude to an empty string as we're reading in 1 byte
        # blocks.
        size_magnitude = ""
    else:
        raise AssertionError(
            "Invalid component size, must be <= the partition size\n"
            "component_size_KiB={}\npartition_size_KiB={}".format(
                component_size / 1024, partition_size
            )
        )
    dd_command = (
        "set -x; "
        r"BD=$(/sbin/blkid -L rootfs1 | sed 's/p[0-9]\+$//'); "
        "OF={ofile}; "
        "SKIP=$(cat {align})K; "
        "COUNT={count}; "
        "dd if=$BD of=$OF skip=$SKIP count=$COUNT bs=1{magnitude} "
        "iflag=skip_bytes".format(
            ofile=output_file_path,
            align=align_file_path,
            count=count,
            magnitude=size_magnitude,
        )
    )
    execute_helper.send_mbl_cli_command(
        ["shell", dd_command], dut_addr, raise_on_error=True
    )
    return output_file_path


def download_from_url(url):
    """Download a file from the url.

    This method is very specific to our LAVA insfrastructure.
    If it fails to download the file, it tries to use the proxy set up in Oulu.
    """
    if not url:
        raise AssertionError("url is an empty string")

    filename = os.path.join(tempfile.mkdtemp(), os.path.basename(url))
    try:
        urllib.request.urlretrieve(url, filename)
    except HTTPError as inst:
        print("\n\nError {}\n\n".format(type(inst)), file=sys.stderr)
        print(
            "urlretrieve failed. Trying alternative method by adding "
            "mapping between 192.168.130.43 and "
            "artifactory-proxy.mbed-linux.arm.com into /etc/hosts.",
            file=sys.stderr,
        )
        try:
            with open("/etc/hosts", "a") as f:
                f.write(
                    "192.168.130.43  " "artifactory-proxy.mbed-linux.arm.com\n"
                )
        except PermissionError as inst:
            print("\n\nError {}\n\n".format(type(inst)), file=sys.stderr)
            print(
                "Could not update /etc/hosts. Perhaps run as root?",
                file=sys.stderr,
            )

        # re-try the urlretrieve.
        try:
            urllib.request.urlretrieve(url, filename)
        except HTTPError as inst:
            print("\n\nError {}\n\n".format(type(inst)), file=sys.stderr)
            print("\n\nAlternative method also failed.\n\n", file=sys.stderr)
            raise
    return filename


def get_dut_address(dut, execute_helper):
    """Discover the IP address of the DUT using mbl-cli."""
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


def get_pelion_device_id(dut_addr, execute_helper):
    """Get the DUT device id by reading the mbl-cloud-client log."""
    _, output, _ = execute_helper.send_mbl_cli_command(
        ["shell", 'grep -i "device id" /var/log/mbl-cloud-client.log'],
        dut_addr,
        raise_on_error=True,
    )

    return output.split("Device Id: ")[1].splitlines()[0]


def get_kernel_version(dut_addr, execute_helper):
    """Get the kernel version running on the DUT using mbl-cli."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'sh -l -c "uname -a"'], dut_addr
    )
    return exit_code, output


def get_file_contents_md5(path, dut_addr, execute_helper):
    """Run md5sum on a file on the DUT."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'sh -l -c "md5sum {}"'.format(path)],
        dut_addr,
        raise_on_error=True,
    )
    return output.splitlines()[1].split()[0]


def get_file_sha256sum(path, dut_addr, execute_helper):
    """Run sha256sum on a file on the DUT."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'sh -l -c "sha256sum {}"'.format(path)],
        dut_addr,
        raise_on_error=True,
    )
    return output.splitlines()[1].split()[0]


def get_mounted_bank_device_name(mount_point, dut_addr, execute_helper):
    """Get the name of the block device mounted at a particular mount."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", '/usr/bin/lsblk --noheadings --output "MOUNTPOINT,KNAME"'],
        dut_addr,
        raise_on_error=True,
    )
    for line in output.splitlines():
        match = re.search(r"^{} .*".format(mount_point), line)
        if match:
            return match.group(0).split()[1]

    raise AssertionError(
        "No match for the mount point found! Output was: {}".format(output)
    )


def get_file_mtime(path, dut_addr, execute_helper):
    """Get the last modified time of a file on the DUT."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", "stat {}".format(path)], dut_addr, raise_on_error=True
    )
    for line in output.splitlines():
        if "Modify:" in line:
            mtime = line
            break
    if not mtime:
        raise AssertionError(
            "Could not find Modified Time in the output from `stat`. "
            "The output was {}".format(output)
        )
    return mtime


def get_app_info(app_name, dut_addr, execute_helper, app_output):
    """Get app info using runc and reading its log file."""
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", "runc state {}".format(app_name)],
        dut_addr,
        raise_on_error=True,
    )
    if app_output:
        exit_code, output_cat, error = execute_helper.send_mbl_cli_command(
            ["shell", "cat /var/log/app/{}.log".format(app_name)],
            dut_addr,
            raise_on_error=True,
        )
        output = "{}{}".format(output, output_cat)
    return output


def strings_grep(dut_addr, execute_helper, file_path, pattern):
    """Run strings command on a file and grep for a pattern using mbl-cli."""
    command = "strings {} | grep {}".format(file_path, pattern)
    exit_code, output, error = execute_helper.send_mbl_cli_command(
        ["shell", 'sh -l -c "{}"'.format(command)],
        dut_addr,
        raise_on_error=True,
    )
    return output.splitlines()[1]


class ExecuteHelper:
    """Class to provide a wrapper for executing commands as a subprocess."""

    debug = False

    def __init__(self, request):
        """Initialise the class."""
        ExecuteHelper.debug = request.config.getoption("--debug-output")

    @staticmethod
    def _print(data):
        if ExecuteHelper.debug:
            print(data)

    @staticmethod
    def execute_command(command, timeout=None):
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
        try:
            output, error = p.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            ExecuteHelper._print("Timed out after {}s".format(timeout))
            p.kill()
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
    def send_mbl_cli_command(command, addr, raise_on_error=False):
        """Execute the provided mbl-cli command.

        Executes the mbl-cli command and returns the error code, stdout and
        stderr.

        Optionally raises AssertionError on a non-zero exit code.
        """
        exit_code = 1
        output = ""
        error = ""

        if not addr:
            raise AssertionError(
                "Invalid DUT address. Given address is an empty string."
            )

        cli_command = ["mbl-cli", "--address", addr] + [x for x in command]
        exit_code, output, error = ExecuteHelper.execute_command(cli_command)
        if raise_on_error and exit_code != 0:
            msg = (
                "mbl-cli command '{cmd}' returned an error code of {code}."
                "\nCommand stdout: {out}\nCommand stderr: {err}\n"
            )
            raise AssertionError(
                msg.format(
                    cmd=cli_command, code=exit_code, out=output, err=error
                )
            )

        return exit_code, output, error
