#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Connection data encapsulation class."""


class Connection:
    """Representation of a single connection."""

    def __init__(self, protocol, ip, port, pid, executable):
        """
        Create and initialize connection object.

        :param protocol: protocol name
        :param ip: IP address string
        :param port: port value
        :param pid: PID of running executable
        :param executable: name of executable process
        """
        assert protocol is not None
        self.protocol = protocol
        self.ip = ip
        self.port = port
        self.pid = pid
        self.executable = executable

    def __str__(self):
        """
        Convert connection object to string.

        :return: string representation of the object
        """
        output = "{},{}:{} {}({})".format(
            self.protocol, self.ip, self.port, self.executable, self.pid
        )
        return output

    def is_equal_port(self, protocol, port):
        """
        Compare if protocol and port values are equual to the object values.

        :param protocol: protocol name
        :param port: port value
        :return: True if both: protocol and port values are equal to the
            object data. Otherwise return False
        """
        assert protocol is not None
        assert port is not None
        return (self.protocol == protocol) and (self.port == port)

    def is_equal_executable(self, executable):
        """
        Compare if executable name is equual to the object value.

        :param executable: executable name
        :return: True if executable name is equal to the object data value.
            Otherwise return False
        """
        assert executable is not None
        return self.executable == executable
