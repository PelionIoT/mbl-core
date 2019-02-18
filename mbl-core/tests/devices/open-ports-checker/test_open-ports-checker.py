#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

import logging
import os
import sys
import pytest
import mbl.open_ports_checker.open_ports_checker as opc

class TestOpenPortsScanner:
    """ Open port scanner main class. """

    @pytest.fixture(autouse=True)
    def setup_port_scanner(self):
        """ Setup open port scaner. """

        info_level = logging.DEBUG
        logging.basicConfig(
            level=info_level,
            format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        )
        logger = logging.getLogger("OpenPortsChecker")
        logger.setLevel(info_level)

    def test_run_port_scanner(self):
        """ Run open port scan test."""

        white_list_file = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "mbl",
            "open_ports_checker",
            "white_list.json",
        )
        open_ports_checker = opc.OpenPortsChecker(white_list_file)
        ret = open_ports_checker.run_check()
        assert ret == opc.Status.SUCCESS

    def test_check_expected_failure(self):
        """
        Run open port scan negative test.
        
        Run open port scan with empty open ports whitelist and make sure test
        failed.
        """

        empry_list_file = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "mbl",
            "open_ports_checker",
            "empty_list.json",
        )
        open_ports_checker = opc.OpenPortsChecker(empry_list_file)
        ret = open_ports_checker.run_check()
        assert ret == opc.Status.BLACK_LISTED_CONNECTION
