#!/usr/bin/env python3

# Copyright (c) 2019, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

"""Check that the requested container goes through the expected states.

The container is expected to be "RUNNING" and then transition to "STOPPED"
some time later.

"""

import sys
import time
import argparse
import mbl.app_lifecycle_manager.app_lifecycle_manager as alm


def setup_parser():
    """Create command line parser.

    :return: parser object.

    """
    parser = argparse.ArgumentParser(description="Check Container Status")
    parser.add_argument(
        "contanerId",
        type=str,
        default="user-sample-app-package",
        help="contaner name",
    )
    return parser


def main():
    """Perform the main execution."""
    # Parse command line
    options = setup_parser().parse_args()

    lava_signal = "<LAVA_SIGNAL_TESTCASE TEST_CASE_ID="
    result_str = "RESULT="
    terminator = ">"

    # Create App LifeCycle Manager
    app_lifecycle_mng = alm.AppLifecycleManager()

    # The application is expected to be already running.
    # Set the timeout to 10 seconds
    endtime = time.monotonic() + 10

    # Find out state.
    state = app_lifecycle_mng.get_container_state(options.contanerId)
    print("{}: {}".format("Current state is", state))

    # Loop until it is in the RUNNING state, or the timeout fires
    while state != alm.ContainerState.RUNNING and endtime > time.monotonic():
        print(
            "{}: {}".format(
                "Waiting for state of Running. Current state is", state
            )
        )

        time.sleep(1)
        state = app_lifecycle_mng.get_container_state(options.contanerId)
        print("{}: {}".format("New state is", state))

    # Did we timeout? Display appropriate result messages
    if endtime < time.monotonic():
        print("Timeout waiting for application to run!!")
        print(
            "{}{}{} {}{}{}".format(
                lava_signal,
                options.contanerId,
                "_Running",
                result_str,
                "fail",
                terminator,
            )
        )
        print(
            "{}{}{} {}{}{}".format(
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
            "{}{}{} {}{}{}".format(
                lava_signal,
                options.contanerId,
                "_Running",
                result_str,
                "pass",
                terminator,
            )
        )

        # The application is running and is expected to run for 20 seconds
        # Set the timeout to 30 seconds to allow a healthy margin
        endtime = time.monotonic() + 30

        # Loop until it is in the STOPPED state, or the timeout fires
        while (
            state != alm.ContainerState.STOPPED and endtime > time.monotonic()
        ):
            print(
                "{}: {}".format(
                    "Waiting for state of Stopped. Current state is", state
                )
            )

            time.sleep(1)
            state = app_lifecycle_mng.get_container_state(options.contanerId)
            print("{}: {}".format("New state is", state))

        # Did we timeout? Display appropriate result messages
        if endtime < time.monotonic():
            print("Timeout waiting for application to stop!!")
            print(
                "{}{}{} {}{}{}".format(
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
                "{}{}{} {}{}{}".format(
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
