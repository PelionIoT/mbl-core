#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""This script checks for checker for unwanted TCP/UDP open ports."""


import os
import json
import logging
from enum import Enum
import mbl.open_ports_checker.connection as connection
import mbl.open_ports_checker.netstatutils as nsu

__version__ = "1.0"


class Status(Enum):
    """OpenPortsChecker operation status codes."""

    SUCCESS = 0
    BLACK_LISTED_CONNECTION = 1


class OpenPortsChecker:
    """Checker for unwanted open ports."""

    def __init__(self, white_list_filename):
        """
        Create and initialize OpenPortsChecker object.

        :param white_list_filename: white list .json file name
        """
        self.logger = logging.getLogger("OpenPortsChecker")
        self.logger.info("Initializing OpenPortsChecker")
        self.logger.info("Version {}".format(__version__))
        # Load connections white list JSON file
        with open(white_list_filename, "r") as in_file:
            self.white_list = json.load(in_file)

    def run_check(self):
        """
        Run open ports check.

        :return: Status.SUCCESS if all open ports are white-listed
                 otherwise Status.BLACK_LISTED_CONNECTION
        """
        active_connections = self.__get_list_of_active_connections()
        self.logger.debug(
            "Found {} active connections".format(len(active_connections))
        )
        return self.__check_connections_against_white_list(active_connections)

    def __check_connection_against_white_list(self, connection):
        """
        Check if a single connection is white listed.

        :param connection: connection objects to be checked against
            white list
        :return: Status.SUCCESS
                 Status.BLACK_LISTED_CONNECTION
        """
        check_result = Status.BLACK_LISTED_CONNECTION
        ports = self.white_list["ports"]
        for port_data in ports:
            protocol = port_data["protocol"]
            port = port_data["port"]
            if connection.is_equal_port(protocol, port):
                check_result = Status.SUCCESS
                break
        executables = self.white_list["executables"]
        for executable_data in executables:
            executable = executable_data["executable"]
            if connection.is_equal_executable(executable):
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
                 Status.BLACK_LISTED_CONNECTION
        """
        self.logger.debug("***Checking connections against white list***")
        blacklisted_connections = 0
        for connection in connections:
            self.logger.debug(
                "Checking connection status: {}".format(connection)
            )
            connection_status = self.__check_connection_against_white_list(
                connection
            )
            self.logger.debug(
                "Connection status: {}".format(connection_status)
            )
            if connection_status != Status.SUCCESS:
                blacklisted_connections += 1
                self.logger.info(
                    "Connection {} is blacklisted".format(connection)
                )
        self.logger.info(
            "Found {}/{} blacklisted connections".format(
                blacklisted_connections, len(connections)
            )
        )
        return (
            Status.SUCCESS
            if blacklisted_connections == 0
            else Status.BLACK_LISTED_CONNECTION
        )

    def __get_list_of_active_connections(self):
        """
        Get list of all active connections except loopback.

        :return: List of active connections
        """
        self.logger.debug("Get list of active connections")
        active_connections = nsu.netstat()
        return active_connections
