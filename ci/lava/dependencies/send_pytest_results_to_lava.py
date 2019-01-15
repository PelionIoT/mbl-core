#!/usr/bin/env python3

import argparse
import os
import re
import sys


def setup_parser():
    parser = argparse.ArgumentParser(description="Send Pytest Results To LAVA")
    parser.add_argument("--input", type=str, default="./results.txt",
                        help="Results file generated by pytest")
    return parser

def main():
    # Parse command line
    options = setup_parser().parse_args()

    # results store
    results = []

    # setup regular expressions for PASSED and FAILED
    pass_re = re.compile('.*::.*::.*PASSED')
    fail_re = re.compile('.*::.*::.*FAILED')

    # Look for the results file and process it if it exists
    if os.path.isfile(options.input):
        file = open(options.input)
        for line in file:
            if(pass_re.search(line)):
                name = line.split("::", 1)[1].split(" ")[0]
                results.append("<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
                               + name
                               + " RESULT=PASS>")
            elif(fail_re.search(line)):
                name = line.split("::", 1)[1].split(" ")[0]
                results.append("<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
                               + name
                               + " RESULT=FAIL>")
            else:
                # skip the line
                pass
    else:
        print("Input File not found")

    for result in results:
        print(result)


if __name__ == "__main__":
    sys.exit(main())
