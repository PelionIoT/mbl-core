"""Copyright (c) 2018 ARM Limited and Contributors. All rights reserved.

SPDX-License-Identifier: Apache-2.0
"""


import sys
import subprocess
import os
import gzip
import tempfile


"""The aim of this file is to test the randomness of the OS random generator.

This is done by reading the special file /dev/random which serves as
pseudorandom number generator.

"""
RANDOMNESS_SOURCE = "/dev/random"

# helper function(s)


def parse_rngtest_output(rngtest_output: str) -> (int, int):
    """Parse rngtest output to return succcess and failure counts."""
    try:
        # Identify the lines that provide information about
        # success(es) and failure(s)
        statistics_line_success = ""
        statistics_line_failure = ""
        substr_success = "rngtest: FIPS 140-2 successes: "
        substr_failure = "rngtest: FIPS 140-2 failures: "

        for line in rngtest_output.split("\n"):
            if substr_success in line:
                statistics_line_success = line
            elif substr_failure in line:
                statistics_line_failure = line

        count_success = int(statistics_line_success.split(":")[-1])
        count_failure = int(statistics_line_failure.split(":")[-1])
        return (count_success, count_failure)
    except Exception:
        print("Unexpected error: {}".format(sys.exc_info()[0]))
        raise


class RandomNumberGeneratorAccess:
    """Allow safe access to random generator with the 'with' statement."""

    def __enter__(self):
        """Open the random generator file and return it."""
        self.bytes_generator = open(RANDOMNESS_SOURCE, "rb")
        return self.bytes_generator

    def __exit__(self, type, value, traceback):
        """Close previously opened random generator file."""
        self.bytes_generator.close()


class TestRandomNumberGenerator:
    """Test method(s) for the random number generator."""

    def test_compression_size(self):
        """Test that there is no repeated pattern in the rng output.

        Test the randomness of the data by ensuring that no redundancy is
        detected after compressing a file containing a number of bytes obtained
        from the random generator source.
        """
        with tempfile.NamedTemporaryFile(mode="wb") as tmpfile:
            with gzip.GzipFile(tmpfile.name, mode="wb") as zipped_tmpfile:
                with RandomNumberGeneratorAccess() as rng:
                    no_of_bytes = 4000
                    zipped_tmpfile.write(rng.read(no_of_bytes))
            assert os.path.getsize(zipped_tmpfile.name) >= no_of_bytes

    def test_fips_140_2(self):
        """
        Test the rng against the FIPS 140-2 security standard.

        The test works on blocks of 20000 bits (2500 bytes), using the
        FIPS 140-2 RNG tests (Monobit test, Poker test, Runs test,cLong run
        test and Continuous test) to test the randomness of each block of data.
        The test is performed by running Linux rngtest tool from a shell
        command via a subprocess. The result from the tool is used to check the
        pass condition.
        """
        num_of_blocks = 5

        # Run Linux 'rngtest' program as a shell subprocess to perform the test
        shell_command = "rngtest -c{} < {}".format(
            num_of_blocks, RANDOMNESS_SOURCE
        )
        process = subprocess.run(
            args=shell_command, shell=True, stderr=subprocess.PIPE
        )

        # Read the program statistics output from stderr
        rngtest_output = process.stderr.decode("utf8")

        # Get the success and failure counts
        count_success, count_failure = parse_rngtest_output(rngtest_output)

        # Check the results
        assert count_success == num_of_blocks
        assert count_failure == 0
