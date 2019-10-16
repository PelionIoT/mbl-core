# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

metadata:
    format: Lava-Test Test Definition 1.0
    name: Mbed-Crypto-Test
    description: Mbed Crypto tests

    parameters:
        #
        # virtual_env: specifies the Python virtual environment
        #
        virtual_env:

run:
    steps:
        - . $virtual_env/bin/activate

        - pytest --verbose -s ./ci/lava/tests/test-mbed-crypto.py
