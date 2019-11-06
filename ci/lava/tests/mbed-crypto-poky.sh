
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

set +x
set -e
# For each test binary there is a corresponding ".datax" file. Enumerate
# the datax files then remove the ".datax" suffix to get a list of tests.
cd /usr/lib/mbed-crypto/test
TESTS=`find . -maxdepth 1 -name \*.datax -type f`
if [ -z "$TESTS" ]; then
    echo "Error: no tests found"
    exit 1
fi

# Run the tests
set +e  # Allow the tests to fail as we capture the result
for test in $TESTS; do
    # Remove the suffix
    test=`basename $test .datax`
    # Run the test and capture the result
    ./$test
    res=$?

    # Print out the result
    echo -n "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
    echo -n `basename $test`
    echo -n " RESULT="
    if [ $res -eq 0 ]; then
        echo -n "pass"
    else
        echo -n "fail"
    fi
    echo ">"
done
