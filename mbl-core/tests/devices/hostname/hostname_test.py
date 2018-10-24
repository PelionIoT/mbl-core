#!/usr/bin/env python3
# Copyright (c) 2018, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""
Pytest for testing hostname script.

"""

import subprocess
import os
from subprocess import check_output

SET_HOSTNAME_CMD = [os.path.join(os.path.sep, "etc", "init.d", "hostname.sh")]
HOSTNAME_CMD = [os.path.join(os.path.sep, "bin", "hostname")]
HOSTNAME_FACTORY_FILE = os.path.join(
    os.path.sep, "config", "factory", "hostname"
)
HOSTNAME_USER_FILE = os.path.join(os.path.sep, "config", "user", "hostname")


class TestHostname:
    """
    Test Hostname main class.

    This class verify factory and user configured hostname.
    Hostname factory configuration, if exist, located in
    /config/factory/hostname. Hostname user configuration, if exist,
    located in /config/user/hostname.
    Before this test run factory config partitions /config/factory
    should be remounted as r/w.
    """

    hostname_factory = None
    hostname_user = None
    hostname_orig = None

    @classmethod
    def setup_class(cls):
        """
        setup_class is invoked once at the beginning of a class test methods.

        """
        result = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        cls.hostname_orig = result.stdout.decode("utf-8").strip()

        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            with open(HOSTNAME_FACTORY_FILE, "r") as f:
                cls.hostname_factory = f.read().strip()

        print(
            "Hostname command returned: {}, "
            "factory defined hostname: {}".format(
                cls.hostname_orig, cls.hostname_factory
            )
        )

        if os.path.isfile(HOSTNAME_USER_FILE):
            with open(HOSTNAME_USER_FILE, "r") as f:
                cls.hostname_user = f.read().strip()
                assert cls.hostname_user == cls.hostname_orig
        else:
            assert cls.hostname_factory == cls.hostname_orig

        print("User defined hostname: {}".format(cls.hostname_user))

    def teardown_method(self, method):
        """
        teardown_method is invoked for every test method of a class.

        """
        # Restore the original hostname configuration
        print("Teardown method start...")
        self._restore_hostname()
        print("Teardown method end")

    # check modify user defined hostname
    def test_hostname_no_factory_modify_user_config(
        self, new_hostname="hostname_test_change_usr_cfg"
    ):
        """ check modify user defined hostname """
        # Setup: remove factory configuration and add user configuration
        #
        self._setup_only_factory_cfg_hostname("hostname_fact_test")

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write(new_hostname)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == new_hostname

    # check add user configuration when factory configuration exist
    def test_hostname_factory_cfg_exist_add_usr_cfg(self):

        # Setup: remove user configuration and add factory configuration
        #
        self._setup_only_factory_cfg_hostname("hostname_fact_test")

        # Test add user configuration
        #
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write(self.hostname_orig)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == self.hostname_orig

    # check modify user configuration when factory configuration exist
    def test_hostname_factory_cfg_exist_modify_usr_cfg(self):

        # Setup: add factory and user configurations
        #
        self._setup_user_and_factory_cfg_hostname(
            "hostname_fact_test", "hostname_usr_test"
        )

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write("hostname_test_modify")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert (
            hostname.stdout.decode("utf-8").strip() == "hostname_test_modify"
        )

    # check remove user configuration when factory configuration exist
    def test_hostname_factory_cfg_exist_remove_user_config(self):

        # Setup: add factory and user configurations
        #
        self._setup_user_and_factory_cfg_hostname(
            "hostname_fact_test", "hostname_usr_test"
        )

        # Test remove user configuration
        #
        os.remove(HOSTNAME_USER_FILE)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == "hostname_fact_test"

    # check remove of user configuration when no factory hostname defined
    def test_hostname_no_factory_cfg_remove_user_config(self):

        # Setup: remove factory and add user hostname configurations
        #
        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write("hostname_usr_test")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == "hostname_usr_test"

        # Test remove user configuration
        #
        os.remove(HOSTNAME_USER_FILE)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").find("mbed_linux_") == 0

    # setup: remove factory configuration and add user configuration
    def _setup_only_factory_cfg_hostname(self, hostname_factory):

        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write(hostname_factory)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == hostname_factory

    # setup: add user and factory configuration
    def _setup_user_and_factory_cfg_hostname(
        self, hostname_factory, hostname_user
    ):

        # Setup: add factory and user configurations
        #
        with open(HOSTNAME_FACTORY_FILE, "w") as f:
            f.write(hostname_factory)
        with open(HOSTNAME_USER_FILE, "w") as f:
            f.write(hostname_user)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == hostname_user

    # restore the original configuration
    def _restore_hostname(self):

        if self.hostname_user is not None:
            with open(HOSTNAME_USER_FILE, "w") as f:
                f.write(self.hostname_user)
        elif os.path.isfile(HOSTNAME_USER_FILE):
            os.remove(HOSTNAME_USER_FILE)

        if self.hostname_factory is not None:
            with open(HOSTNAME_FACTORY_FILE, "w") as f:
                f.write(self.hostname_factory)
        elif os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)

        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode("utf-8").strip() == self.hostname_orig

    # run /etc/init.d/hostname.sh
    def _run_hostname_script(self):

        p = subprocess.Popen(
            SET_HOSTNAME_CMD, 
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE
        )
        try:
            output, error = p.communicate()
        finally:
            subprocess._cleanup()
            p.stdout.close()
            p.stderr.close()

        print(
            "hostname.sh returned: {}, output: {}, error: {}".format(
                p.returncode, output.decode("utf-8"), error.decode("utf-8")
            )
        )

