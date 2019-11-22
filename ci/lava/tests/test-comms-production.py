#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to validate mDNS and SSH connection in production images."""

from datetime import datetime
import re
import time


class TestCommsProductionImage:
    """Class to test the mDNS and SSH connections in production."""

    # Address set via test_mDNS_responder()
    dut_address = ""
    # List of DUT connections set via test_DUT_addresses()
    dut_connections = {}

    def test_mDNS_responder(self, execute_helper, dut):
        """Perform mDNS responder test.

        Run avahi-browse -tpr _ssh._tcp and you should have only one reply
        from the DUT on the gadget interface.

        Use "dut" from fixture test options to limit browse (for developers).

        """
        devices_list = []
        start_time = datetime.now()
        elapsed_time = 0
        five_minutes = 60 * 5
        dut_filter = dut if dut != "auto" else "mbed-linux-os-"

        while elapsed_time < five_minutes and len(devices_list) == 0:

            devices_list = self._run_avahi_browse(
                execute_helper, dut_filter=dut_filter
            )
            # Pause between commands
            time.sleep(5)
            # calculate long have we been running.
            elapsed_time = (datetime.now() - start_time).seconds

        # Set up DUT address for later tests
        if len(devices_list) != 0:
            if devices_list[0][1] == "IPv6":
                TestCommsProductionImage.dut_address = "{}%{}".format(
                    devices_list[0][3], devices_list[0][0]
                )
            else:
                TestCommsProductionImage.dut_address = devices_list[0][3]

        # Expect only one device
        assert len(devices_list) == 1

    def _run_avahi_browse(
        self, execute_helper, dut_filter="", address_filter=""
    ):
        devices_list = []

        test_command = ["avahi-browse", "-tpr", "_ssh._tcp"]
        err, device_list, error = execute_helper.execute_command(
            test_command, 60
        )
        if err == 0:
            # Expecting lines like '=;enp0s20u1u1;IPv6;mbed-linux-os-9709;
            #   SSH Remote Terminal;local;mbed-linux-os-9709.local;
            #   fe80::ec12:dcff:fe8c:3cd7;22;"mblos"'
            # Which contain  with interface[0], IP version[1], hostname[2]
            # and IP address[3]
            reg = r"^=;([^;]*);([^;]*);({}[^;]*);[^;]*;[^;]*;[^;]*;({}[^;]*)"
            results = re.findall(
                reg.format(dut_filter, address_filter), device_list, re.M
            )
            if results:
                devices_list = results
                for device in devices_list:
                    print(
                        "Browse: {}/{} - {} ({})".format(
                            device[0], device[1], device[2], device[3]
                        )
                    )

        return devices_list

    def test_DUT_addresses(self, execute_helper):
        """Perform test of other DUT addresses on host.

        Check that the addresses available on the DUT are not
        present in the avahi-browse list.

        """
        assert TestCommsProductionImage.dut_address != ""

        # Get all connections available on DUT
        conns = self._get_dut_connections(execute_helper)
        TestCommsProductionImage.dut_connections = conns
        assert TestCommsProductionImage.dut_connections

        # Review all interface connections for addresses we can find in avahi
        for (
            interface,
            addresses,
        ) in TestCommsProductionImage.dut_connections.items():
            for version, address in addresses.items():
                # Don't look up local addresses or the one we used to connect
                if not TestCommsProductionImage.dut_address.startswith(
                    address
                ) and (
                    (version == "inet" and address != "127.0.0.1")
                    or (version == "inet6" and not address.startswith("::"))
                ):
                    print("Checking {}".format(address))
                    # Perform avahi browse with retries so that we can ignore
                    # avahi-browse failures
                    retries = 3
                    dev_list = []
                    while retries > 0 and len(dev_list) == 0:
                        dev_list = self._run_avahi_browse(
                            execute_helper, address_filter=address
                        )
                        retries = -1
                    if dev_list:
                        print(
                            "Incorrectly found {} in avahi browse".format(
                                address
                            )
                        )
                    assert not dev_list

    def _get_dut_connections(self, execute_helper):
        """Use mbl-cli ssh via USB to get interfaces and addresses."""
        print("Connecting to {}".format(TestCommsProductionImage.dut_address))
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            ["shell", "sh -c -l 'ip addr'"],
            TestCommsProductionImage.dut_address,
        )
        connections = {}
        if err == 0:
            # Parse ip addr output
            interface = ""
            for line in stdout.split("\n"):
                # Expects: '6: usbdbg0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu
                #   1500 qdisc pfifo_fast qlen 100'
                match = re.match(r"[0-9]+:\s+([^:]*):.*", line)
                if match:
                    interface = match.group(1)
                    continue
                # Expects: 'inet6 fe80::ec12:dcff:fe8c:3cd7/64 scope link'
                # or: 'inet 127.0.0.1/8 scope host lo'
                match = re.match(r"\s+(inet[6]*)\s+([^/]*)/[0-9]*.*", line)
                if match:
                    # Create interface dictionary for this network type
                    # (inet or inet6)
                    if interface not in connections:
                        connections[interface] = {}
                    connections[interface][match.group(1)] = match.group(2)
            print("Connections dictionary: {}".format(connections))
        else:
            print(
                "Error {} connecting to DUT\n{}{}".format(err, stdout, stderr)
            )
        return connections

    def _check_SSH_port(self, execute_helper, address):
        """Check for SSH port on IPv4 address with timeout."""
        test_command = ["nc", "-vzw", "3", address, "22"]
        err, device_list, error = execute_helper.execute_command(
            test_command, 60
        )
        return err == 0

    def test_SSH_via_IPv4(self, execute_helper):
        """Perform a test of SSH on all IPv4 connections.

        SSH using anything but USB gadget shouldn't work.

        """
        assert TestCommsProductionImage.dut_connections
        ssh_addresses = []
        for (
            interface,
            addresses,
        ) in TestCommsProductionImage.dut_connections.items():
            for version, address in addresses.items():
                if version == "inet" and address != "127.0.0.1":
                    ssh = self._check_SSH_port(execute_helper, address)
                    if ssh:
                        ssh_addresses.append(address)
                        print(
                            "SSH incorrectly available on {}".format(address)
                        )
        assert len(ssh_addresses) == 0
