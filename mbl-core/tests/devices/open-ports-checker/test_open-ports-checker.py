#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import mbl.open_ports_checker.open_ports_checker as opc
import logging
import os


class TestOpenPortsScanner:
    """ Open port scanner main class. """

    def test_run_port_scanner(self):
        """ Run open port scan test."""

        info_level = logging.DEBUG

        logging.basicConfig(
            level=info_level,
            format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        )
        white_list_file = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "mbl",
            "open_ports_checker",
            "white_list.json",
        )
        open_ports_checker = opc.OpenPortsChecker(white_list_file)
        ret = open_ports_checker.run_check()
        assert ret == opc.Status.SUCCESS
