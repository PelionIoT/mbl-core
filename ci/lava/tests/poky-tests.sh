
# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

set +x
set -e
TESTS="poky-psa-trusted-storage-linux.sh poky-mbed-crypto.sh"
# Disabled until PSA Arch Tests version works with latest PSA API version
# poky-psa-arch-tests.sh

# Run the tests
set +e  # Allow the tests to fail as we capture the result
result=0
for test in $TESTS; do
    test="./ci/lava/tests/${test}"
    # Run the test and capture the result
    $test
    ret=$?

    if [ $ret -ne 0 ]; then
        result=$ret
    fi
done
