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

    app_lifecycle_mng = alm.AppLifecycleManager()

    endtime = time.monotonic()  + 10

    state = app_lifecycle_mng.get_container_state(options.contanerId)

    print(state)

    while ( state != alm.ContainerState.RUNNING and endtime > time.monotonic()):
        print("Waiting for Running")

        time.sleep(1)
        state = app_lifecycle_mng.get_container_state(options.contanerId)
        print(state)

    if endtime < time.monotonic():
        print("Timeout waiting for application to run!!")
    else:
        print("Application detected as Running")

        endtime = time.monotonic() + 30

        while ( state != alm.ContainerState.STOPPED and endtime > time.monotonic()):
            print("Waiting for Stopped")

            time.sleep(1);
            state = app_lifecycle_mng.get_container_state(options.contanerId)

        if endtime < time.monotonic():
            print("Timeout waiting for application to stop!!")
        else:
            print("Application detected as Stopped")

if __name__ == "__main__":
    sys.exit(main())

