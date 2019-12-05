/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * UpdateD (pronounced update-dee) is a system daemon that coordinates updates
 * to device firmware.  Updates will consist of bundles of one or more
 * firmware packages that should be applied as a set.  The delivery of updates
 * to a device will be the responsibility of an external delivery mechanism.
 * UpdateD runs in the background on a device and listens for update requests.
 * When a request is received, UpdateD will hand off responsibility for applying
 * the update to swupdate. */

#include <unistd.h>

int main()
{
    while(1)
    {
        pause();
    }

    return 0;
}
