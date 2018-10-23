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
    os.path.join(os.path.sep, 'etc', 'init.d','hostname.sh')
]
HOSTNAME_CMD = [os.path.join(os.path.sep, 'bin', 'hostname')]
HOSTNAME_FACTORY_FILE = os.path.join(os.path.sep, 'config', 'factory',
    'hostname')
HOSTNAME_USER_FILE = os.path.join(os.path.sep, 'config', 'user',
    'hostname')
RM_HOSTNAME_USER_FILE_CMD = [os.path.join(os.path.sep, 'bin', 'rm'),
    HOSTNAME_USER_FILE]
RM_HOSTNAME_FACTORY_FILE_CMD = [os.path.join(os.path.sep, 'bin', 'rm'),
    HOSTNAME_FACTORY_FILE]


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

        if os.path.isfile(
            HOSTNAME_FACTORY_FILE
        ) and os.path.getsize(HOSTNAME_FACTORY_FILE):

            with open(HOSTNAME_FACTORY_FILE, 'r') as f:
                hostname_factory = f.read().rstrip()

        print(
            "Hostname command returned: {}, "
            "factory defined hostname: {}".format(
                hostname_orig,
                hostname_factory
            )
        )

        if os.path.isfile(HOSTNAME_USER_FILE) and \
            os.path.getsize(HOSTNAME_USER_FILE):
				
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

        self._check_hostname_remove_user_config(
            hostname_factory, hostname_user, hostname_orig
        )
        self._check_hostname_modify_user_config(
            hostname_factory, hostname_user, hostname_orig
        )   
        self._check_hostname_factory_config_exist(
            hostname_factory, hostname_user, hostname_orig
        )

    # check modify user defined hostname
    def _check_hostname_modify_user_config(
        self, hostname_factory, hostname_user, hostname_orig, 
        new_hostname="hostname_test_change_usr_cfg"
    ):

        # Setup: remove factory configuration and add user configuration
        #
        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            subprocess.check_call(RM_HOSTNAME_FACTORY_FILE_CMD)
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write(new_hostname)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == new_hostname

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_change_user_config")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == \
            "hostname_change_user_config"

        # Restore the original hostname configuration
        #
        self._restore_hostname(
            hostname_factory, hostname_user, hostname_orig
        )

    # check add/modify/remove user configuration when factory 
    # configuration exist
    def _check_hostname_factory_config_exist(
        self, hostname_factory, hostname_user, hostname_orig, 
        new_hostname="hostname_test_factory_cfg"
    ):

        # Setup: remove user configuration and add factory configuration
        #
        if os.path.isfile(HOSTNAME_USER_FILE):
            subprocess.check_call(RM_HOSTNAME_USER_FILE_CMD)
        with open(HOSTNAME_FACTORY_FILE, 'w') as f:
            f.write(new_hostname)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == new_hostname

        # Test add user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write(hostname_orig)        
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == hostname_orig

        # Test modify user configuration
        #
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_test_modify")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == \
            "hostname_test_modify"

        # Test remove user configuration
        #
        subprocess.check_call(RM_HOSTNAME_USER_FILE_CMD)
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == new_hostname        

        # Restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)

    # check remove of user configuration when no factory hostname defined
    def _check_hostname_remove_user_config(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        # Setup: remove factory configuration and add user configuration
        #
        if os.path.isfile(HOSTNAME_FACTORY_FILE):
            subprocess.check_call(RM_HOSTNAME_FACTORY_FILE_CMD)
        with open(HOSTNAME_USER_FILE, 'w') as f:
            f.write("hostname_test")
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == "hostname_test"

        # Test remove user configuration
        #
        subprocess.check_call(RM_HOSTNAME_USER_FILE_CMD)  
        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').find("mbed_linux_") == 0

        # restore the original hostname configuration
        #
        self._restore_hostname(hostname_factory, hostname_user, hostname_orig)    

    # restore the original configuration
    def _restore_hostname(
        self, hostname_factory, hostname_user, hostname_orig
    ):

        if hostname_user is not None:
            with open(HOSTNAME_USER_FILE, 'w') as f:
                f.write(hostname_user)
        elif os.path.isfile(HOSTNAME_USER_FILE):
            subprocess.check_call(RM_HOSTNAME_USER_FILE_CMD)

        if hostname_factory is not None:
            with open(HOSTNAME_FACTORY_FILE, 'w') as f:
                f.write(hostname_factory)
        elif os.path.isfile(HOSTNAME_FACTORY_FILE):
            subprocess.check_call(RM_HOSTNAME_FACTORY_FILE_CMD)

        self._run_hostname_script()
        hostname = subprocess.run(HOSTNAME_CMD, stdout=subprocess.PIPE)
        assert hostname.stdout.decode('utf-8').rstrip() == hostname_orig     

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
