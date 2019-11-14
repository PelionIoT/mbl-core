
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

set +x
set -e

cd /usr/bin
# List of test executables
TESTS="psa-storage-example-app"

# Run the tests
set +e  # Allow the tests to fail as we capture the result
for test in $TESTS; do
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
