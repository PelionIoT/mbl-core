# Copyright (c) 2018 ARM Ltd.


import subprocess
import re
import os
import scipy.stats
import numpy
import zipfile

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


def extract_count_from_str(
    stats_str: str,
    stats_preceding_str: str
) -> str:
    """ Extracts the count from a line of a rngtest statistics output."""
    count = re.search(
        pattern="{}(.*)$".format(stats_preceding_str),
        string=stats_str
    ).group(1)
    return count


def run_chi_squared_goodness_of_fit(
) -> (numpy.float64, numpy.float64):
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

    # Create a random number generator object.
    rng = RandomNumberGeneratorService()

    # Create a list that will contain the number of occurrences for
    # each number between 0 - 255 i.e cells[n] contains the number of
    # occurences of n.
    cells = []

    # Initialise the list with zeros.
    for _ in range(num_cell):
        cells.append(0)

    # Set the occurrences of each numbers between 0 and 255.
    for n in range(0, no_of_random_bytes):
        cells[rng.generate_random_byte()] += 1

    # Calculate the chisquare and p-value.
    chi2, p = scipy.stats.chisquare(f_obs=cells)
    return (chi2, p)


class RandomNumberGeneratorService():
    """The service class to generate random bytes."""

    def __init__(
        self,
        source: str = RANDOMNESS_SOURCE
    ) -> int:
        self.set_bytes_generator(source)

    def set_bytes_generator(
        self,
        source: str = RANDOMNESS_SOURCE
    ):
        self.src = source
        self.bytes_generator = open(source, "rb")

    def generate_random_byte(
        self
    ) -> list:
        """Generate a byte."""
        return int.from_bytes(
            self.bytes_generator.read(1),
            byteorder='big'
        )

    def generate_n_random_bytes(
        self,
        n: int=1
    ):
        """Generate a number of bytes."""
        bytes_ls = []
        while len(self.numbers) < n:
            byte = self.generate_random_byte()
            bytes_ls.append(byte)
        return bytes_ls


class TestRandomNumberGenerator():
    """Test method(s) for the random number generator."""

    def test_compression_size(
        self
    ):
        """Test the randomness of the data by ensuring that no redundancy is
           detected after compressing a file containing a number of bytes
           obtained from the random generator source.
        """
        # Create file to zip
        unzipped_file = "random_bytes.txt"
        no_of_bytes = 4000
        rng = RandomNumberGeneratorService()
        f = open(unzipped_file, "wb+")
        for _ in range(0, no_of_bytes):
            f.write(rng.bytes_generator.read(1))
        f.close()

        # Zip file
        zipped_file = "random_bytes.zip"
        zipfile.ZipFile(zipped_file, mode='w').write(unzipped_file)

        # Check result
        assert os.path.getsize(zipped_file) >= os.path.getsize(unzipped_file)

        # Clean up
        os.remove(unzipped_file)
        os.remove(zipped_file)

    def test_null_hypothesis(
        self
    ):
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
        count_p_value_acceptable = 0

        for _ in range(num_of_static_test_run):
            p_value = (run_chi_squared_goodness_of_fit()[1])
            if not (p_value <= low_cutoff) and not (p_value >= high_cutoff):
                count_p_value_acceptable += 1

        # Check the results.
        assert count_p_value_acceptable >= (num_of_static_test_run/2)

    def test_fips_140_2(
        self
    ):
        """The test works on blocks of 20000 bits (2500 bytes), using the
           FIPS 140-2 RNG tests (Monobit test, Poker test, Runs test,
           Long run test and Continuous test) to test the randomness of each
           block of data.
        """
        num_of_blocks = 5
        randomness_source = "/dev/random"

        # Run Linux 'rngtest' program as a shell subprocess to perform the test
        bash_command = "rngtest -c{} < {}".format(
            num_of_blocks,
            randomness_source
        )
        process = subprocess.run(
            args=bash_command,
            shell=True,
            stderr=subprocess.PIPE
        )

        # Read the program statistics output from stderr
        statistics_output = process.stderr.decode('utf8')

        # Identify the lines that provide information about
        # success(es) and failure(s)
        statistics_line_success = []
        statistics_line_failure = []
        substr_success = "rngtest: FIPS 140-2 successes: "
        substr_failure = "rngtest: FIPS 140-2 failures: "
        for line in statistics_output.split('\n'):
            if substr_success in line:
                statistics_line_success = line
            elif substr_failure in line:
                statistics_line_failure = line

        # Get the success and failure counts
        count_success = int(
            extract_count_from_str(
                stats_str=statistics_line_success,
                stats_preceding_str=substr_success
            )
        )
        count_failure = int(
            extract_count_from_str(
                stats_str=statistics_line_failure,
                stats_preceding_str=substr_failure
            )
        )

        # Check the results
        assert count_success == num_of_blocks
        assert count_failure == 0
