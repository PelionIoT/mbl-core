#!/usr/bin/env python3
# Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Script to run tests on the DUT."""

import pytest
import re


class TestRunner:
    """Class to encapsulate the testing on a DUT."""

    dut_address = ""

    def test_setup_dut_addr(self, dut_addr):
        """Store the device address."""
        TestRunner.dut_address = dut_addr

        assert dut_addr != ""

    @pytest.mark.parametrize(
        "test_action",
        [
            "test_suite_aes.cbc",
            "test_suite_aes.cfb",
            "test_suite_aes.ecb",
            "test_suite_aes.ofb",
            "test_suite_aes.rest",
            "test_suite_aes.xts",
            "test_suite_arc4",
            "test_suite_aria",
            "test_suite_asn1write",
            "test_suite_base64",
            "test_suite_blowfish",
            "test_suite_camellia",
            "test_suite_ccm",
            "test_suite_chacha20",
            "test_suite_chachapoly",
            "test_suite_cipher.aes",
            "test_suite_cipher.arc4",
            "test_suite_cipher.blowfish",
            "test_suite_cipher.camellia",
            "test_suite_cipher.ccm",
            "test_suite_cipher.chacha20",
            "test_suite_cipher.chachapoly",
            "test_suite_cipher.des",
            "test_suite_cipher.gcm",
            "test_suite_cipher.misc",
            "test_suite_cipher.nist_kw",
            "test_suite_cipher.null",
            "test_suite_cipher.padding",
            "test_suite_cmac",
            "test_suite_ctr_drbg",
            "test_suite_des",
            "test_suite_dhm",
            "test_suite_ecdh",
            "test_suite_ecdsa",
            "test_suite_ecjpake",
            "test_suite_ecp",
            "test_suite_entropy",
            "test_suite_error",
            "test_suite_gcm.aes128_de",
            "test_suite_gcm.aes128_en",
            "test_suite_gcm.aes192_de",
            "test_suite_gcm.aes192_en",
            "test_suite_gcm.aes256_de",
            "test_suite_gcm.aes256_en",
            "test_suite_gcm.camellia",
            "test_suite_gcm.misc",
            "test_suite_hkdf",
            "test_suite_hmac_drbg.misc",
            "test_suite_hmac_drbg.no_reseed",
            "test_suite_hmac_drbg.nopr",
            "test_suite_hmac_drbg.pr",
            "test_suite_md",
            "test_suite_mdx",
            "test_suite_memory_buffer_alloc",
            "test_suite_mpi",
            "test_suite_nist_kw",
            "test_suite_oid",
            "test_suite_pem",
            "test_suite_pk",
            "test_suite_pkcs1_v15",
            "test_suite_pkcs1_v21",
            "test_suite_pkcs5",
            "test_suite_pkparse",
            "test_suite_pkwrite",
            "test_suite_poly1305",
            "test_suite_psa_crypto",
            "test_suite_psa_crypto_entropy",
            "test_suite_psa_crypto_hash",
            "test_suite_psa_crypto_init",
            "test_suite_psa_crypto_metadata",
            "test_suite_psa_crypto_persistent_key",
            "test_suite_psa_crypto_se_driver_hal",
            "test_suite_psa_crypto_se_driver_hal_mocks",
            "test_suite_psa_crypto_slot_management",
            "test_suite_psa_its",
            "test_suite_rsa",
            "test_suite_shax",
            "test_suite_timing",
            "test_suite_version",
            "test_suite_xtea",
        ],
    )
    def test_runner(self, execute_helper, test_action):
        """Perform the test on the DUT via the mbl-cli."""
        # Check status
        err, stdout, stderr = execute_helper.send_mbl_cli_command(
            [
                "shell",
                "cd /usr/lib/mbed-crypto/test && ./{}".format(test_action),
            ],
            TestRunner.dut_address,
        )
        assert err == 0
