# Copyright (c) 2018 ARM Ltd.


import sys
import subprocess
import os
import scipy.stats
import numpy
import gzip
import tempfile

""" Pytest tests

Pytest is the framework used to run tests. It autodiscovers tests by the
name of the class or method/function. More info about autodiscovery:
https://docs.pytest.org/en/latest/goodpractices.html#test-discovery

Pytest documentation can be found here: https://docs.pytest.org/en/latest/

The aim of this file is to test the randomness of the OS random generator.

This is done by reading the special file /dev/random which serves as
pseudorandom number generator.

"""
RANDOMNESS_SOURCE = "/dev/random"

# helper function(s)


def parse_rngtest_output(rngtest_output: str) -> (int, int):
    """
    Extract the success and failure counts from a rngtest statistics output.
    """
    try:
        # Identify the lines that provide information about
        # success(es) and failure(s)
        statistics_line_success = ""
        statistics_line_failure = ""
        substr_success = "rngtest: FIPS 140-2 successes: "
        substr_failure = "rngtest: FIPS 140-2 failures: "

        for line in rngtest_output.split('\n'):
            if substr_success in line:
                statistics_line_success = line
            elif substr_failure in line:
                statistics_line_failure = line

        count_success = int(statistics_line_success.split(":")[-1])
        count_failure = int(statistics_line_failure.split(":")[-1])
        return (count_success, count_failure)
    except BaseException:
        print("Unexpected error: {}".format(sys.exc_info()[0]))
        raise


def run_chi_squared_goodness_of_fit() -> (numpy.float64, numpy.float64):
    """Perform Hypothesis testing to determine with a certain
    level of certainty that the hypothesis that the data
    is taken from a uniform distribution can be rejected.
    Return the chi square and the p-value in that order.
    """
    # Number of possible numbers or cells or bins is 256 because a
    # random byte between 0 and 255 is generated.
    num_cell = 256

    # Number of times the rng will be run.
    no_of_random_bytes = 25600

    with RandomNumberGeneratorAccess() as rng:
        # Create a list that will contain the number of occurrences for
        # each number between 0 - 255 i.e cells[n] contains the number of
        # occurences of n.
        cells = []

        # Initialise the list with zeros.
        for _ in range(num_cell):
            cells.append(0)

        # Set the occurrences of each numbers between 0 and 255.
        for n in range(0, no_of_random_bytes):
            integer = int.from_bytes(rng.read(1), byteorder="big")
            cells[integer] += 1

        # Calculate the chisquare and p-value.
        chi2, p = scipy.stats.chisquare(f_obs=cells)
        return (chi2, p)


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
        """Test the randomness of the data by ensuring that no redundancy is
        detected after compressing a file containing a number of bytes
        obtained from the random generator source.
        """
        with tempfile.NamedTemporaryFile(mode="wb") as tmpfile:
            with gzip.GzipFile(tmpfile.name, mode="wb") as zipped_tmpfile:
                with RandomNumberGeneratorAccess() as rng:
                    no_of_bytes = 4000
                    zipped_tmpfile.write(rng.read(no_of_bytes))
            assert os.path.getsize(zipped_tmpfile.name) >= no_of_bytes

    def test_distribution_null_hypothesis(self):
        """The test performs hypothesis testing to determine with a certain
        level of certainty that the hypothesis that the data
        is taken from a uniform distribution can be rejected. This is done
        by running an hypothesis test multiple times and analysing the
        probability of observing a test statistic at least as extreme in
        a chi-squared distribution, also known as p-value.
        A pass is determined when the majority of p-values fall between
        chosen low cutoff and high cutoff value.
        """
        # Set up test parameters
        low_cutoff = 0.05
        high_cutoff = 0.95
        num_of_static_test_run = 100
        num_acceptable_p_value_for_pass = 78
        count_p_value_acceptable = 0

        #
        for _ in range(num_of_static_test_run):
            p_value = (run_chi_squared_goodness_of_fit()[1])
            if not (p_value <= low_cutoff) and not (p_value >= high_cutoff):
                count_p_value_acceptable += 1

        # Check the results.
        assert count_p_value_acceptable >= num_acceptable_p_value_for_pass

    def test_fips_140_2(self):
        """
        The test works on blocks of 20000 bits (2500 bytes), using the
        FIPS 140-2 RNG tests (Monobit test, Poker test, Runs test,
        Long run test and Continuous test) to test the randomness of each
        block of data.
        """
        num_of_blocks = 5

        # Run Linux 'rngtest' program as a shell subprocess to perform the test
        shell_command = "rngtest -c{} < {}".format(
            num_of_blocks,
            RANDOMNESS_SOURCE
        )
        process = subprocess.run(
            args=shell_command,
            shell=True,
            stderr=subprocess.PIPE
        )

        # Read the program statistics output from stderr
        rngtest_output = process.stderr.decode("utf8")

        # Get the success and failure counts
        count_success, count_failure = parse_rngtest_output(rngtest_output)

        # Check the results
        assert count_success == num_of_blocks
        assert count_failure == 0


#if __name__ == "__main__":
#    test = TestRandomNumberGenerator()
#    test.test_random_streams_generated()
