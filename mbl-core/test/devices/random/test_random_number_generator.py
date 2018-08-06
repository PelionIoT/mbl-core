# Copyright (c) 2018 ARM Ltd.


import os
import scipy.stats
import numpy

""" Pytest tests

Pytest is the framework used to run tests. It autodiscovers tests by the name
of the class or method/function. More info about autodiscovery:
https://docs.pytest.org/en/latest/goodpractices.html#test-discovery

Pytest documentation can be found here: https://docs.pytest.org/en/latest/

The aim of this file is to test the randomness of the OS random generator.
This is done by reading the special file /dev/random which serves as
pseudorandom number generator.

"""
DEFAULT_RANDOMNESS_SOURCE = "/dev/random"


class RandomNumberGeneratorService():
    """The service class to generate random numbers."""

    def __init__(
        self,
        source: str = DEFAULT_RANDOMNESS_SOURCE
    ) -> int:
        self.numbers = []
        self.set_bytes_generator(source)

    def set_bytes_generator(
        self,
        source: str = DEFAULT_RANDOMNESS_SOURCE
    ):
        self.src = source
        self.bytes_generator = open(source, "rb")

    def generate_random_int(
        self
    ) -> list:
        """Generate an integer value."""
        return int.from_bytes(
            self.bytes_generator.read(1),
            byteorder='big'
        )

    def generate_n_random_ints(
        self,
        n: int = 1
    ):
        """Generate a number of integers."""
        while len(self.numbers) < n:
            number = self.generate_random_int()
            self.numbers.append(number)


class TestRandomNumberGenerator():
    """Test the random number generator."""

    def test_random_streams_generated(
        self
    ):
        """The test ensures that the random number generator produces
           a random stream. This is done by running an hypothesis test
           multiple times and analysing the probability of observing a test
           statistic at least as extreme in a chi-squared distribution, also
           known as p-value.
           A pass is determined when the majority of p-values fall between
           chosen low cutoff and high
           cutoff value
        """
        # Set up test parameters
        low_cutoff = 0.05
        high_cutoff = 0.95
        num_of_static_test_run = 100
        count_p_value_acceptable = 0

        #
        for _ in range(num_of_static_test_run):
            p_value = (self.run_chi_squared_goodness_of_fit_on_rng()[1])
            if not (p_value <= low_cutoff) and not (p_value >= high_cutoff):
                count_p_value_acceptable += 1

        # Check the results.
        assert count_p_value_acceptable >= (num_of_static_test_run/2)

    # helper functions
    def run_chi_squared_goodness_of_fit_on_rng(
        self
    ) -> (numpy.float64, numpy.float64):
        """Perform Hypothesis testing to determine with a certain
           level of certainty that the hypothesis that the data
           is taken from a uniform distribution can be rejected.
           Return the chi square and the p-value in that order.
        """
        # Number of possible numbers or cells or bins is 256 because a
        # random byte between 0 and 255 is generated.
        num_cell = 256

        # Create a random number generator object.
        rng = RandomNumberGeneratorService()

        # Run the random number generator multiple time.
        rng.generate_n_random_ints(n=num_cell*100)

        # Create a list that will contain the number of occurrences for
        # each number between 0 - 255 i.e cells[n] contains the number of
        # occurences of n.
        cells = []

        # Initialise the list with zeros.
        for _ in range(num_cell):
            cells.append(0)

        # Set the occurrences of each numbers between 0 and 255.
        for n in range(0, len(rng.numbers)):
            cells[rng.numbers[n]] += 1

        # Calculate the chisquare and p-value.
        chi2, p = scipy.stats.chisquare(f_obs=cells)
        return (chi2, p)
