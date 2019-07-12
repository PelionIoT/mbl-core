#!/bin/sh

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

set +x
psa-arch-crypto-tests > /tmp/results
state="LookingForTest"
while read a
do
    if [ $state == "LookingForTest" ];
    then
        if `echo $a | grep -q "DESCRIPTION:"`
        then
            test=`echo $a  | cut -d":" -f2-`
            test=${test/ /}
            test=${test// /_}
            test=${test//_|_DESCRIPTION:/}
            state="LookingForResult"
        fi
    elif [ $state = "LookingForResult" ];
    then
        if `echo $a | grep -q "RESULT"`
        then
            echo -n "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
            echo -n $test
            echo -n " RESULT="
            if `echo $a | grep -q "PASS"`
            then
                echo -n "pass"
            else
                echo -n "fail"
            fi
            echo ">"
            state="LookingForTest"
       fi
   fi
- done < /tmp/results


