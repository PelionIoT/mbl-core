/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 *
 * This utility initialises the hardware watchdog and sets the timeout.
 * The program also tries to determine the last boot reason so we can tell if
 * the last reboot was caused by the hardware watchdog.
 * This code is intended for use with our robust update mechanism; we need
 * to start the watchdog before we mount the rootfs, so the device reboots if
 * loading the rootfs fails.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

//============================================================================
// Error handling convenience functions
int is_err(const int ret)
{
    return ret == -1;
}


int handle_errno(const char *message)
{
    int err_code = errno;
    perror(message);
    return err_code;
}

//============================================================================
// Watchdog functions
int set_watchdog_timeout(const int watchdog_fd, int *timeout)
{
    return ioctl(watchdog_fd, WDIOC_SETTIMEOUT, timeout);
}


int get_last_boot_status(const int watchdog_fd, int *flags)
{
    return ioctl(watchdog_fd, WDIOC_GETBOOTSTATUS, flags);
}


int print_last_boot_reason(const int boot_status)
{
    switch (boot_status)
    {
        case WDIOF_OVERHEAT:
            printf("The last reboot was caused by the CPU overheating.\n");
            return 0;
        case WDIOF_CARDRESET:
            printf("The last reboot was caused by a watchdog reset.\n");
            return 0;
    }
    return -1;
}

//============================================================================
// Main entry point
int main(int argc, char **argv)
{
    int timeout = 0;
    char* wdog_filename = "";

    // Parse command line arguments
    int current_opt;
    int optindex;
    struct option longopts[] = {
        {"timeout", required_argument, NULL, 't'},
        {"wdog-fname", required_argument, NULL, 'w'},
    };

    while ((current_opt = getopt_long(argc, argv, "tw:", longopts, &optindex)) != -1)
    {
        switch (current_opt)
        {
            case 't':
                timeout = atoi(optarg);
                break;
            case 'w':
                wdog_filename = optarg;
                break;
            default:
                printf("--timeout and --wdog-fname must be provided. Exiting.\n");
                return EXIT_FAILURE;
        }
    }

    const int wdog_fd = open(wdog_filename, O_WRONLY);
    if (is_err(wdog_fd))
    {
        return handle_errno("Failed to open watchdog device file");
    }

    int flags;
    const int gs_ret = get_last_boot_status(wdog_fd, &flags);
    if (is_err(gs_ret))
    {
        perror("Failed to get the last boot status");
    }
    else
    {
        const int lb_ret = print_last_boot_reason(flags);
        if (is_err(lb_ret))
        {
            printf("Failed to determine last boot reason.\n");
        }
    }

    const int to_ret = set_watchdog_timeout(wdog_fd, &timeout);
    if (is_err(to_ret))
    {
        return handle_errno("Failed to set the watchdog timeout");
    }

    const int cl_ret = close(wdog_fd);
    if (is_err(cl_ret))
    {
        return handle_errno("Failed to close the watchdog device file");
    }

    return EXIT_SUCCESS;
}
