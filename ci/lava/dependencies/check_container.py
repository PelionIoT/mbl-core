#!/usr/bin/env python3

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause


import sys
import time
import argparse
import mbl.app_lifecycle_manager.app_lifecycle_manager as alm


def setup_parser():
    """Create command line parser.

    :return: parser object.

    """
    parser = argparse.ArgumentParser(description="Check Contaier Status")
    parser.add_argument(
        "contanerId", type=str, default="user-sample-app-package", help="contaner name"
    )
    return parser


def main():
    """Perform the main execution."""
    # Parse command line
    options = setup_parser().parse_args()

    lava_signal = "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
    result_str = "RESULT="
    terminator = ">"

    app_lifecycle_mng = alm.AppLifecycleManager()

    endtime = time.monotonic() + 10

    state = app_lifecycle_mng.get_container_state(options.contanerId)
    print("{}: {}".format("Current state is", state))

    while state != alm.ContainerState.RUNNING and endtime > time.monotonic():
        print("{}: {}".format("Waiting for state of Running. Current state is", state))

        time.sleep(1)
        state = app_lifecycle_mng.get_container_state(options.contanerId)
        print("{}: {}".format("New state is", state))

    if endtime < time.monotonic():
        print("Timeout waiting for application to run!!")
        print(
            "{}{}{} {}{}".format(
                lava_signal,
                options.contanerId,
                "_Running",
                result_str,
                "fail",
                terminator,
            )
        )
        print(
            "{}{}{} {}{}".format(
                lava_signal,
                options.contanerId,
                "_Stopped",
                result_str,
                "skip",
                terminator,
            )
        )
    else:
        print("Application detected as Running")
        print(
            "{}{}{} {}{}".format(
                lava_signal,
                options.contanerId,
                "_Running",
                result_str,
                "pass",
                terminator,
            )
        )

        endtime = time.monotonic() + 30

        while state != alm.ContainerState.STOPPED and endtime > time.monotonic():
            print(
                "{}: {}".format("Waiting for state of Stopped. Current state is", state)
            )

            time.sleep(1)
            state = app_lifecycle_mng.get_container_state(options.contanerId)
            print("{}: {}".format("New state is", state))

        if endtime < time.monotonic():
            print("Timeout waiting for application to stop!!")
            print(
                "{}{}{} {}{}".format(
                    lava_signal,
                    options.contanerId,
                    "_Stopped",
                    result_str,
                    "fail",
                    terminator,
                )
            )
        else:
            print("Application detected as Stopped")
            print(
                "{}{}{} {}{}".format(
                    lava_signal,
                    options.contanerId,
                    "_Stopped",
                    result_str,
                    "pass",
                    terminator,
                )
            )


if __name__ == "__main__":
    sys.exit(main())
