"""Copyright (c) 2018 ARM Limited and Contributors. All rights reserved.

SPDX-License-Identifier: Apache-2.0
"""


import sys
import subprocess
import os
# import scipy.stats
import numpy
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


# def run_chi_squared_goodness_of_fit() -> (numpy.float64, numpy.float64):
#     """Calculate the chi-square and p-value for a rng sample.

#     Perform Hypothesis testing to determine with a certain level of certainty
#     that the hypothesis that the random number generator data is taken from a
#     uniform distribution can be rejected.
#     Return the chi square and the p-value in that order.
#     """
#     # Number of possible numbers or cells or bins is 256 because a
#     # random byte between 0 and 255 is generated.
#     num_cell = 256

#     # Number of times the rng will be run.
#     no_of_random_bytes = 25600

#     with RandomNumberGeneratorAccess() as rng:
#         # Create a list that will contain the number of occurrences for
#         # each number between 0 - 255 i.e cells[n] contains the number of
#         # occurences of n.
#         cells = []

#         # Initialise the list with zeros.
#         cells = [0] * num_cell

#         # Set the occurrences of each numbers between 0 and 255.
#         for n in range(0, no_of_random_bytes):
#             integer = int.from_bytes(rng.read(1), byteorder="big")
#             cells[integer] += 1

#         # Calculate the chisquare and p-value.
#         chi2, p = scipy.stats.chisquare(f_obs=cells)
#         return (chi2, p)


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

    # def test_distribution_null_hypothesis(self):
    #     """
    #     Run multiple chi-square tests on a rng to assess its distribution.

    #     The test performs hypothesis testing to determine with a certain
    #     level of certainty that the hypothesis that the data is taken from a
    #     uniform distribution can be rejected. This is done by running an
    #     hypothesis test multiple times and analysing the probability of
    #     observing a test statistic at least as extreme in a chi-squared
    #     distribution, also known as p-value.A pass is determined when the
    #     majority of p-values fall between chosen low cutoff and high cutoff
    #     value. The low cutoff in this case is the significance level, which is
    #     the level of p-value required to reject the null hypothesis. Typically
    #     researchers choose a significance level equal to 0.01, 0.05, or 0.10;
    #     0.05 was chosen here. The high cutoff value (corresponding to a low
    #     chi-square value) indicates a high correlation between the sample of
    #     the rng and a uniform distribution. Researchers usually use 0.95 as the
    #     high cutoff value.
    #     The total number of "good" p-values required to pass was set to 78% as
    #     the probability that a TRNG would fail (based on chosen low and high
    #     cutoff values) is 0.00011415631973171919. A TRNG would be expected to
    #     to fail the test about once every 10,100 runs.
    #     """
    #     # Set up test parameters
    #     low_cutoff = 0.05
    #     high_cutoff = 0.95
    #     num_of_static_test_run = 100
    #     num_acceptable_p_value_for_pass = 78
    #     count_p_value_acceptable = 0

    #     for _ in range(num_of_static_test_run):
    #         p_value = run_chi_squared_goodness_of_fit()[1]
    #         if low_cutoff <= p_value <= high_cutoff:
    #             count_p_value_acceptable += 1

    #     # Check the results.
    #     assert count_p_value_acceptable >= num_acceptable_p_value_for_pass

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
