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

HOSTNAME_SCRIPT_CMD = [
    os.path.join(os.path.sep, 'etc', 'init.d', 'hostname.sh')
]
HOSTNAME_CMD = [os.path.join(os.path.sep, 'bin', 'hostname')]
HOSTNAME_FACTORY_FILE = os.path.join(
    os.path.sep, 'config', 'factory', 'hostname'
)
HOSTNAME_USER_FILE = os.path.join(
    os.path.sep, 'config', 'user', 'hostname'
)


class TestHostname:
    """Test Hostname main class."""

    def test_main(self):
        """
        Test main function.

        This test verify factory and user configured hostname.
        Hostname factory configuration, if exist, located in
        /config/factory/hostname. Hostname user configuration, if exist,
        located in /config/user/hostname.
        Before this test run factory config partitions /config/factory
        should be remounted as r/w.

        :return: None
        """
        hostname_factory = None
        hostname_user = None

        result = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        hostname_orig = result.stdout.decode('utf-8').rstrip()

        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            with open(HOSTNAME_FACTORY_FILE, 'r') as f:
                hostname_factory = f.read().rstrip()

        print(
            "Hostname command returned: {}, "
            "factory defined hostname: {}".format(
                hostname_orig,
                hostname_factory
            )
        )

        if os.path.isfile(HOSTNAME_USER_FILE):
            with open(HOSTNAME_USER_FILE, 'r') as f:
                hostname_user = f.read().rstrip()
                assert hostname_user == hostname_orig
        else:
            assert hostname_factory == hostname_orig

        print(
            "User defined hostname: {}".format(
                hostname_user
            )
        )

        self._check_hostname_no_factory_cfg_remove_user_config(
            hostname_factory, hostname_user, hostname_orig
        )
        self._check_hostname_no_factory_modify_user_config(
            hostname_factory, hostname_user, hostname_orig
        )
        
        self._check_hostname_factory_cfg_exist_add_usr_cfg(
            hostname_factory, hostname_user, hostname_orig
        )
        self._check_hostname_factory_cfg_exist_modify_usr_cfg(
            hostname_factory, hostname_user, hostname_orig
        )
        self._check_hostname_factory_cfg_exist_remove_user_config(
            hostname_factory, hostname_user, hostname_orig
        )

    # check modify user defined hostname
    def _check_hostname_no_factory_modify_user_config(
        self, hostname_factory, hostname_user, hostname_orig,
        new_hostname="hostname_test_change_usr_cfg"
    ):

        # Setup: remove factory configuration and add user configuration
        #
        self._setup_only_factory_cfg_hostname()

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write(new_hostname)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == new_hostname

        # Restore the original hostname configuration
        #
        self._restore_hostname(
            hostname_factory, hostname_user, hostname_orig
        )

    # check add user configuration when factory configuration exist
    def _check_hostname_factory_cfg_exist_add_usr_cfg(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        # Setup: remove user configuration and add factory configuration
        #
        self._setup_only_factory_cfg_hostname()

        # Test add user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write(hostname_orig)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == hostname_orig

        # Restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)
  
    # check modify user configuration when factory configuration exist
    def _check_hostname_factory_cfg_exist_modify_usr_cfg(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        # Setup: add factory and user configurations
        #
        self._setup_user_and_factory_cfg_hostname(
            "hostname_fact_test", "hostname_usr_test"
        )

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_test_modify")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == \
            "hostname_test_modify"

        # Restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)
  
    # check remove user configuration when factory configuration exist
    def _check_hostname_factory_cfg_exist_remove_user_config(
        self, hostname_factory, hostname_user, hostname_orig
    ):

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
        assert hostname.stdout.decode('utf-8').strip() == "hostname_fact_test"

        # Restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)

    # check remove of user configuration when no factory hostname defined
    def _check_hostname_no_factory_cfg_remove_user_config(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        # Setup: remove factory and add user hostname configurations
        #
        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_usr_test")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == "hostname_usr_test"

        # Test remove user configuration
        #
        os.remove(HOSTNAME_USER_FILE)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').find("mbed_linux_") == 0

        # restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)

    # setup: remove factory configuration and add user configuration
    def _setup_only_factory_cfg_hostname(
        self
    ):

        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_test")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == "hostname_test"

    # setup: add user and factory configuration
    def _setup_user_and_factory_cfg_hostname(
        self, hostname_factory, hostname_user
    ):

        # Setup: add factory and user configurations
        #
        with open(HOSTNAME_FACTORY_FILE, 'w') as f:
            f.write(hostname_factory)
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write(hostname_user)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == "hostname_usr_test"

    # restore the original configuration
    def _restore_hostname(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        if hostname_user is not None:
            with open(HOSTNAME_USER_FILE, 'w') as f:
                f.write(hostname_user)
        elif os.path.isfile(HOSTNAME_USER_FILE):
            os.remove(HOSTNAME_USER_FILE)

        if hostname_factory is not None:
            with open(HOSTNAME_FACTORY_FILE, 'w') as f:
                f.write(hostname_factory)
        elif os.path.isfile(HOSTNAME_FACTORY_FILE):
            os.remove(HOSTNAME_FACTORY_FILE)

        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').strip() == hostname_orig

    # run /etc/init.d/hostname.sh
    def _run_hostname_script(
        self
    ):

        p = subprocess.Popen(
            HOSTNAME_SCRIPT_CMD, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        try:
            output, error = p.communicate()
        finally:
            subprocess._cleanup()
            p.stdout.close()
            p.stderr.close()

        print(
                "hostname.sh returned: {}, output: {}, error: {}".format(
                    p.returncode,
                    output.decode("utf-8"),
                    error.decode("utf-8"),
                )
            )
