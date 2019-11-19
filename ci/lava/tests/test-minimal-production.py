#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Pytest for testing a minimal rootfs on production.

It does this by checking that programs or services are not available.
"""

import pytest
import re


"""Commands not checked for as installed by other packages
- see JIRA IOTMBL-2381
    "blkid",  # util-linux installed
    "depmod",  # kmod installed
    "fsck",  # util-linux installed
    "getty",  # agetty installed
    "groups",  # shadow installed
    "insmod",  # kmod installed
    "login",  # shadow installed
    "lsmod",  # kmod installed
    "modprobe",  # kmod installed
    "mount",  # util-linux installed
    "passwd",  # shadow installed
    "reboot",  # systemctl installed
    "rmmod",  # kmod installed
    "su",  # shadow installed
    "sulogin",  # util-linux installed
    "umount",  # util-linux installed
    "fsck.ext2",  # systemd installed ext2 feature from e2fsprogs
    "mkfs.ext2",  # cloud-client installed ext2 feature from e2fsprogs

Features not checked for as no obvious extra packages installed
    keyboard
    screen
    touchscreen
"""


class TestMinimalProductionImage:
    """Class to encapsulate the testing on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestMinimalProductionImage.dut_address = dut_addr

        assert dut_addr != ""

    @pytest.mark.parametrize("service", ["apmd", "neard"])  # apm, nfc features
    def test_find_service(self, execute_helper, service):
        """Check for systemd service on the DUT via the mbl-cli."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "sh -c -l 'systemctl status {}'".format(service)],
            TestMinimalProductionImage.dut_address,
        )
        if err != 4:
            print(
                "Not expecting systemd service: {}".format(
                    stdout.split("\n")[1]
                )
            )
        assert err == 4

    # List of programs that should not be installed on production
    @pytest.mark.parametrize(
        "program",
        [
            "/lib/systemd/systemd-vconsole-setup",  # systemd vconsole support
            "addgroup",
            "adduser",
            "alsactl",  # alsa feature from alsa-utils-alsactl
            "alsamixer",  # alsa feature from alsa-utils-alsamixer
            "apm",  # apm feature
            "chattr",
            "chgrp",
            "chmod",
            "chown",
            "chvt",
            "deallocvt",
            "delgroup",
            "deluser",
            "diff",
            "dumpkmap",
            "dumpleases",
            "fbset",
            "fstrim",
            "fuser",
            "hexdump",
            "hwclock",
            "id",
            "ifdown",
            "ifup",
            "irattach",  # irda feature from irdautils
            "irdaping",  # irda feature from irdautils
            "loadfont",
            "loadkmap",
            "logname",
            "losetup",
            "lspci",  # pci feature
            "mdev",
            "mesg",
            "microcom",
            "mknod",
            "mkswap",
            "mountpoint",
            "nc",
            "nproc",
            "nslookup",
            "openvt",
            "patch",
            "pccardctl",  # pcmcia feature from pcmciautils
            "pidof",
            "pivot-root",
            "pulseaudio",  # pulseaudio feature
            "rdate",
            "renice",
            "resize",
            "rfkill",
            "run-parts",
            "setconsole",
            "setsid",
            "start-stop-daemon",
            "swapoff",
            "swapon",
            "tftp",
            "traceroute",
            "udhcpc",
            "udhcpd",
            "users",
            "vlock",
            "volumeid",
            "whoami",
        ],
    )
    def test_find_program(self, execute_helper, program):
        """Try to find the program on the DUT via the mbl-cli."""
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "sh -c -l 'which {}'".format(program)],
            TestMinimalProductionImage.dut_address,
        )
        if err == 0:
            print("Not expecting program: {}".format(stdout.split("\n")[1]))
        assert err != 0
