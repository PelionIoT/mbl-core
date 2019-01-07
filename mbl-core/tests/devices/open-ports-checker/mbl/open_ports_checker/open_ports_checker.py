#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""This script checks for checker for unwanted TCP/UDP open ports."""

import subprocess
import ipaddress
import json
import logging
import socket
from enum import Enum

# psutil module is optional and may not be available.
# This module implements two classes: OpenPortsCheckerPsutil and
# OpenPortsCheckerNetstat. For OpenPortsCheckerPsutil class the psutil
# module is mandatory, but if psutil module is not available,
# it is still possible to use OpenPortsCheckerNetstat class that provide less
# functionality vs. OpenPortsCheckerPsutil class.
# in order to install psutil module on a device, need to modify
# <beta-mbl repo>/recipes-core/packagegroups/packagegroup-mbl-development.bb
# file and add line
# PACKAGEGROUP_MBL_DEVELOPMENT_PKGS_append = " python3-psutil"
# to it.
# Note: if virtualenv is used on a device, need to use the following
# command in order to include psutil module into the virtualenv environment:
# python3 -m venv my_venv --system-site-packages
try:
    import psutil
except ImportError:
    pass

__version__ = '1.0'


class Status(Enum):
    """OpenPortsChecker operation status codes."""

    SUCCESS = 0
    ERROR_BLACK_LISTED_CONNECTION = 1


class Connection:
    """Representation of a single connection"""

    def __init__(self, protocol, ip, port, executable=None):
        """
        Create and initialize connection object.
        :param protocol: protocol name
        :param ip: IP address string
        :param port: port value
        :param executable: name of executable process including PID
        """

        # str() convert string to unicode in python3
        self.protocol = str(protocol)
        self.ip = str(ip)
        self.port = str(port)
        self.executable = executable

    def __str__(self):
        """
        Convert connection object to string.
        :return: string representation of the object
        """

        if self.executable is None:
            output = '{},{}:{}'.format(self.protocol, self.ip, self.port)
        else:
            output = '{},{}:{} ({})'.format(
                self.protocol,
                self.ip,
                self.port,
                self.executable
            )
        return output

    def is_equal(self, protocol, port):
        """
        Compare if protocol and port values are equual to the object values.

        :param protocol: protocol name
        :param port: port value
        :return: True if both: protocol and port values are equal to the
            object data. Otherwise return False
        """

        # Convert to unicode before comparison in order to prevent mixing
        # between unicode and non-unicode strings
        return (self.protocol == str(protocol)) and (self.port == str(port))


class OpenPortsChecker:
    """
    Checker for unwanted open ports.

    This is an abstract class, protected method
    _get_list_of_active_connections method should be implemented by
    derived class
    """

    def __init__(self, ports_white_list_filename):
        """
        Create and initialize OpenPortsChecker object.

        :param ports_white_list_filename: ports white list .json file name
        """
        self.logger = logging.getLogger('OpenPortsChecker')
        self.logger.info(
            'Version {}'.format(__version__)
        )
        # Load connections white list JSON file
        with open(ports_white_list_filename, 'r') as in_file:
            self.ports_white_list = json.load(in_file)

    def run_check(self):
        """
        Run open ports check.

        In case of failure an error code will return.
        :return: Status.SUCCESS
                 Status.ERROR_BLACK_LISTED_CONNECTION
        """
        active_connections = self._get_list_of_active_connections()
        self.logger.debug(
            'Found {} active connections'.format(len(active_connections))
        )
        return self.__check_connections_against_white_list(active_connections)

    def __check_connection_against_white_list(self, connection):
        """
        Check if a single connection is white listed.

        :param connection: connection objects to be checked against
            white list
        :return: Status.SUCCESS
                 Status.ERROR_BLACK_LISTED_CONNECTION
        """
        check_result = Status.ERROR_BLACK_LISTED_CONNECTION
        ports = self.ports_white_list['ports']
        for port in ports:
            protocol = port['protocol']
            port = port['port']
            if connection.is_equal(protocol, port):
                check_result = Status.SUCCESS
                break
        return check_result

    def __check_connections_against_white_list(self, connections):
        """
        Check list of connections against white list.

        If all connections are listed into white list, the function
        returns Status.SUCCESS overwise an error code will be returned.
        :param connections: list of connections objects to be checked against
            white list
        :return: Status.SUCCESS
                 Status.ERROR_BLACK_LISTED_CONNECTION
        """
        self.logger.debug('***Checking connections against white list***')
        blacklisted_connections = 0
        for connection in connections:
            self.logger.debug(
                'Checking connection status: {}'.format(connection)
            )
            connection_status =\
                self.__check_connection_against_white_list(connection)
            self.logger.debug(
                'Connection status: {}'.format(connection_status)
            )
            if connection_status != Status.SUCCESS:
                blacklisted_connections += 1
                self.logger.info(
                    'Connection {} is blacklisted'.format(connection)
                )
        self.logger.info(
            'Found {}/{} blacklisted connections'.format(
                blacklisted_connections,
                len(connections)
            )
        )
        return Status.SUCCESS if blacklisted_connections == 0 \
            else Status.ERROR_BLACK_LISTED_CONNECTION


class OpenPortsCheckerNetstat(OpenPortsChecker):
    """
    Checker for unwanted open ports.

    Implementation using netstat command.
    """
    def __init__(self, ports_white_list_filename):
        """
        Create and initialize OpenPortsCheckerNetstat object.

        :param ports_white_list_filename: ports white list .json file name
        """
        OpenPortsChecker.__init__(self, ports_white_list_filename)
        self.logger = logging.getLogger('OpenPortsChecker')
        self.logger.info(
            'Initializing OpenPortsCheckerNetstat'
        )

    def _get_list_of_active_connections(self):
        """
        Get list of all active connections except loopback.

        :return: List of active connections
        """
        self.logger.debug('Get list of active connections')
        active_connections = []
        # Get list of all active IPv4 and IPv6 connections
        # using netstat command
        command = ['netstat', '-tunl']
        self.logger.debug(
            "Executing command: {}".format(" ".join(command))
        )
        raw_connections = subprocess.check_output(
            command
        ).decode('utf-8')
        self.logger.debug('Raw connections list:\n {}'.format(raw_connections))

        # Parse netstat raw command output
        all_connections = list(raw_connections.split("\n"))
        # First two lines of the netstat command output are header that
        # should be ignored
        for connection in all_connections[2:]:
            connection_data = connection.split()
            if len(connection_data) == 0:
                # Skip last empty line
                continue
            self.logger.debug('Found connection: {}'.format(connection_data))
            # Parse connection data
            protocol = connection_data[0]
            ip, port = connection_data[3].rsplit(':', 1)
            # str() convert string to unicode in python3
            if ipaddress.ip_address(str(ip)).is_loopback:
                # Skip all loopback connections
                continue
            my_connection = Connection(protocol, ip, port)
            self.logger.debug(
                'Adding connection to the connections list: {}'
                .format(my_connection)
            )
            active_connections.append(my_connection)
        return active_connections


class OpenPortsCheckerPsutil(OpenPortsChecker):
    """
    Checker for unwanted open ports.

    Implementation using psutil library.
    """

    def __init__(self, ports_white_list_filename):
        """
        Create and initialize OpenPortsCheckerNetstat object.

        :param ports_white_list_filename: ports white list .json file name
        """
        OpenPortsChecker.__init__(self, ports_white_list_filename)
        self.logger = logging.getLogger('OpenPortsChecker')
        self.logger.info(
            'Initializing OpenPortsCheckerPsutil'
        )

    def _get_list_of_active_connections(self):
        """
        Get list of all active connections except loopback.

        :return: List of active connections
        """
        active_connections = []
        all_connections = psutil.net_connections('inet')
        for connection in all_connections:
            self.logger.debug('Found connection: {}'.format(connection))
            if connection.type == socket.SOCK_STREAM and\
                    connection.status == psutil.CONN_LISTEN:
                # TCP listening connection
                protocol = 'tcp'
            elif connection.type == socket.SOCK_DGRAM:
                # UDP connection
                protocol = 'udp'
            else:
                continue
            if connection.family == socket.AF_INET6:
                protocol += '6'
            (ip, port) = connection.laddr
            # str() convert string to unicode in python3
            if ipaddress.ip_address(str(ip)).is_loopback:
                # Skip all loopback connections
                continue

            executable = None
            if connection.pid is not None:
                executable_cmdline = psutil.Process(connection.pid).cmdline()
                executable = 'PID:{}, exe:{}'.format(
                    connection.pid,
                    executable_cmdline
                )
            my_connection = Connection(protocol, ip, port, executable)
            self.logger.debug(
                'Adding connection to the connections list: {}'
                .format(my_connection)
            )
            active_connections.append(my_connection)
        return active_connections
