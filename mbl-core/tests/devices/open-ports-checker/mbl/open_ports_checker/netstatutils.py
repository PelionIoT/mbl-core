#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""This script provides network statistics functionality."""

import os
import ipaddress
import codecs
import socket
import struct
from enum import Enum
import mbl.open_ports_checker.connection as connection

# List of TCP state values can be found here:
# https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/net/tcp_states.h?id=HEAD
TCP_STATES = {
    "01": "ESTABLISHED",
    "02": "SYN_SENT",
    "03": "SYN_RECV",
    "04": "FIN_WAIT1",
    "05": "FIN_WAIT2",
    "06": "TIME_WAIT",
    "07": "CLOSE",
    "08": "CLOSE_WAIT",
    "09": "LAST_ACK",
    "0A": "LISTEN",
    "0B": "CLOSING",
}

PROC_ENTRY = os.path.join(os.sep, "proc")
PROC_ENTRY_TCP4 = os.path.join(PROC_ENTRY, "net", "tcp")
PROC_ENTRY_UDP4 = os.path.join(PROC_ENTRY, "net", "udp")
PROC_ENTRY_TCP6 = os.path.join(PROC_ENTRY, "net", "tcp6")
PROC_ENTRY_UDP6 = os.path.join(PROC_ENTRY, "net", "udp6")
PROC_ENTRY_PACKET = os.path.join(PROC_ENTRY, "net", "packet")


class ProcFileSystem(Enum):
    """Data indexes under /proc file system."""

    NET_LOCAL_ADDRESS = 1
    NET_TCP_STATE = 3
    NET_PACKET_INODE = 8
    NET_INODE = 9


def hex_to_dec(s):
    """
    Convert hexadecimal string to decemal format.

        :param s: string in hexadecimal format
        :return: string in decimal format
    """
    return str(int(s, 16))


def parse_ipv4_address(ipv4_hex):
    """
    Convert /proc IPv4 hex address into standard IPv4 notation.

        :param ipv4_hex: IPv4 string in hex format
        :return: IPv4Address object
    """
    ipv4 = int(ipv4_hex, 16)
    # pack IPv4 address in system native byte order, 4-byte integer
    packed = struct.pack("=L", ipv4)
    # convert the IPv4 address from binary to text form
    ascii_ipv4_address = socket.inet_ntop(socket.AF_INET, packed)
    return ipaddress.ip_address(ascii_ipv4_address)


def parse_ipv6_address(ipv6_hex):
    """
    Convert /proc IPv6 hex address into standard IPv6 notation.

        :param ipv6_hex: IPv6 string in hex format
        :return: IPv6Address object
    """
    # convert IPv6 address in ASCII hex format into binary format
    binary = codecs.decode(ipv6_hex, "hex")
    # unpack IPv6 address into 4 32-bit integers in big endian / network
    # byte order
    unpacked = struct.unpack("!LLLL", binary)
    # re-pack IPv6 address as 4 32-bit integers in system native byte order
    repacked = struct.pack("@IIII", *unpacked)
    # convert the IPv6 address from binary to text form
    ascii_ipv6_address = socket.inet_ntop(socket.AF_INET6, repacked)
    return ipaddress.ip_address(ascii_ipv6_address)


def parse_ipv4_address_port_pair(raw_address):
    """
    Convert /proc IPv4 raw address-port pair to human readable format.

        :param raw_address: IPv4 address:port pair in /proc raw format
        :return: IPv4 address
        :return: port number
    """
    host, port = raw_address.split(":")
    return parse_ipv4_address(host), hex_to_dec(port)


def parse_ipv6_address_port_pair(raw_address):
    """
    Convert /proc IPv6 raw address-port pair to human readable format.

        :param raw_address: IPv6 address:port pair in /proc raw format
        :return: IPv6 address
        :return: port number
    """
    host, port = raw_address.split(":")
    return parse_ipv6_address(host), hex_to_dec(port)


def load_socket_table(proc_entry):
    """
    Read the table of network sockets.

    Read the raw table of network sockets from /proc/net.
        :param proc_entry: PROC_ENTRY_XXX file system entry
        :return: Socket table in raw format
    """
    with open(proc_entry, "r") as f:
        content = f.readlines()
        # Remove the header
        content.pop(0)
    return content


def get_pids_from_inode(inode):
    """
    Retrieve list of PIDs from inode.

    Check every running process and find which process using the given inode.
    Note: If process that have an open socket is forked, more than one
    processes may use the same inode.
        :param inode: inode
        :return: List of PIDs
    """
    inode_pids = []
    # set up a list object to store valid pids
    pids = [pid for pid in os.listdir(PROC_ENTRY) if pid.isdigit()]
    # remove the PID of the current python process
    pids.remove(str(os.getpid()))

    for pid in pids:
        # check the /proc/<PID>/fd directory for socket information
        proc_fds_entry = os.path.join(PROC_ENTRY, pid, "fd")
        try:
            fds = os.listdir(proc_fds_entry)
            for fd in fds:
                proc_fd_entry = os.path.join(proc_fds_entry, fd)
                # save the pid for sockets matching our inode
                socket_inode_link = "socket:[{}]".format(inode)
                if socket_inode_link == os.readlink(proc_fd_entry):
                    inode_pids.append(pid)
        except OSError:
            # OSError may happen if one of processes is terminated
            # during operation. Ignoring
            pass
    return inode_pids


def get_exe_name(pid):
    """
    Read process executable name from /proc/<PID>/exe entry.

        :param pid: PID value
        :return: executable name or None if cannot read the executable name
    """
    try:
        exe_name = os.readlink(os.path.join(PROC_ENTRY, pid, "exe"))
    except OSError:
        exe_name = None
    return exe_name


def netstat_tcp4():
    """
    List IPv4 TCP connections.

    Return list of processes utilizing IPv4 TCP listening connections.
    All localhost connections are filtered out.

        :return: List of connections
    """
    tcp4_socket_table = load_socket_table(PROC_ENTRY_TCP4)
    result = []
    for line in tcp4_socket_table:
        socket_data = line.split()
        if not socket_data:
            continue
        # Convert IPv4 address and port from hex to human readable format
        raw_address = socket_data[ProcFileSystem.NET_LOCAL_ADDRESS.value]
        local_address, local_port = parse_ipv4_address_port_pair(raw_address)
        if local_address.is_loopback:
            # Skip all loopback connections
            continue
        tcp_state_hex = socket_data[ProcFileSystem.NET_TCP_STATE.value]
        tcp_state = TCP_STATES[tcp_state_hex]
        if tcp_state != "LISTEN":
            # Skip all non-listenning connections
            continue
        # Extract inode to get PIDs
        inode = socket_data[ProcFileSystem.NET_INODE.value]
        # Get PIDs from inode
        pids = get_pids_from_inode(inode)

        for pid in pids:
            exe_name = get_exe_name(pid)
            my_connection = connection.Connection(
                "tcp4", local_address, local_port, pid, exe_name
            )
            result.append(my_connection)
    return result


def netstat_udp4():
    """
    List IPv4 UDP connections.

    Return list of processes utilizing IPv4 UDP connections.
    All localhost connections are filtered out.

        :return: List of connections
    """
    udp4_socket_table = load_socket_table(PROC_ENTRY_UDP4)

    result = []
    for line in udp4_socket_table:
        socket_data = line.split()
        if not socket_data:
            continue
        # Convert IPv4 address and port from hex to human readable format
        raw_address = socket_data[ProcFileSystem.NET_LOCAL_ADDRESS.value]
        local_address, local_port = parse_ipv4_address_port_pair(raw_address)
        if local_address.is_loopback:
            # Skip all loopback connections
            continue
        # Extract inode to get PIDs
        inode = socket_data[ProcFileSystem.NET_INODE.value]
        # Get PIDs from inode
        pids = get_pids_from_inode(inode)

        for pid in pids:
            exe_name = get_exe_name(pid)
            my_connection = connection.Connection(
                "udp4", local_address, local_port, pid, exe_name
            )
            result.append(my_connection)
    return result


def netstat_tcp6():
    """
    List IPv6 TCP connections.

    Return list of processes utilizing IPv6 TCP listening connections.
    All localhost connections are filtered out.

        :return: List of connections
    """
    tcp6_socket_table = load_socket_table(PROC_ENTRY_TCP6)
    result = []
    for line in tcp6_socket_table:
        socket_data = line.split()
        if not socket_data:
            continue
        # Convert IPv6 address and port from hex to human readable format
        raw_address = socket_data[ProcFileSystem.NET_LOCAL_ADDRESS.value]
        local_address, local_port = parse_ipv6_address_port_pair(raw_address)
        if local_address.is_loopback:
            # Skip all loopback connections
            continue
        tcp_state_hex = socket_data[ProcFileSystem.NET_TCP_STATE.value]
        tcp_state = TCP_STATES[tcp_state_hex]
        if tcp_state != "LISTEN":
            # Skip all non-listenning connections
            continue
        # Extract inode to get PIDs
        inode = socket_data[ProcFileSystem.NET_INODE.value]
        # Get PIDs from inode
        pids = get_pids_from_inode(inode)

        for pid in pids:
            exe_name = get_exe_name(pid)
            my_connection = connection.Connection(
                "tcp6", local_address, local_port, pid, exe_name
            )
            result.append(my_connection)
    return result


def netstat_udp6():
    """
    List IPv6 UDP connections.

    Return list of processes utilizing IPv6 UDP connections.
    All localhost connections are filtered out.

        :return: List of connections
    """
    udp6_socket_table = load_socket_table(PROC_ENTRY_UDP6)

    result = []
    for line in udp6_socket_table:
        socket_data = line.split()
        if not socket_data:
            continue
        # Convert IPv6 address and port from hex to human readable format
        raw_address = socket_data[ProcFileSystem.NET_LOCAL_ADDRESS.value]
        local_address, local_port = parse_ipv6_address_port_pair(raw_address)
        if local_address.is_loopback:
            # Skip all loopback connections
            continue
        # Extract inode to get PIDs
        inode = socket_data[ProcFileSystem.NET_INODE.value]
        # Get PIDs from inode
        pids = get_pids_from_inode(inode)

        for pid in pids:
            exe_name = get_exe_name(pid)
            my_connection = connection.Connection(
                "udp6", local_address, local_port, pid, exe_name
            )
            result.append(my_connection)
    return result


def netstat_raw():
    """
    List raw sockets connections.

    Return list of processes utilizing packet sockets.
    All localhost connections are filtered out.

        :return: List of connections
    """
    raw_socket_table = load_socket_table(PROC_ENTRY_PACKET)

    result = []
    for line in raw_socket_table:
        socket_data = line.split()
        if not socket_data:
            continue
        # Extract inode to get PIDs
        inode = socket_data[ProcFileSystem.NET_PACKET_INODE.value]
        # Get PIDs from inode
        pids = get_pids_from_inode(inode)

        for pid in pids:
            exe_name = get_exe_name(pid)
            my_connection = connection.Connection(
                "raw", None, None, pid, exe_name
            )
            result.append(my_connection)
    return result


def netstat():
    """
    Return list of all connections.

    Return list of TCP listenning connections and UDP connections.
    All localhost connections are filtered out.

    This script must run as root in order to be able to obtain PID values
    of all processes. For more information see:
    https://unix.stackexchange.com/questions/226276/read-proc-to-know-if-a-process-has-opened-a-port
        :return: List of connections
    """
    uid = os.getuid()
    assert uid == 0, "This script must run as root"
    tcp4_list = netstat_tcp4()
    udp4_list = netstat_udp4()
    tcp6_list = netstat_tcp6()
    udp6_list = netstat_udp6()
    raw_list = netstat_raw()
    return tcp4_list + udp4_list + tcp6_list + udp6_list + raw_list
