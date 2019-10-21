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
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

//============================================================================
// Error handling and logging convenience functions
int is_err(const int ret)
{
    return ret == -1;
}


int log_error(const char *message)
{
    int err_code = errno;
    char *prefix = "WATCHDOG-ERROR:";
    char result[strlen(prefix) + strlen(message)];

    snprintf(result, sizeof(result), "%s %s", prefix, message);
    perror(result);

    return err_code;
}


void log_warning(const char *message)
{
    char *prefix = "WATCHDOG-WARNING:";
    printf("%s %s\n", prefix, message);
}


void log_info(const char *message)
{
    char *prefix = "WATCHDOG-INFO:";
    printf("%s %s\n", prefix, message);
}

//============================================================================
// Watchdog functions
int set_watchdog_timeout(const int watchdog_fd, int *timeout)
{
    return ioctl(watchdog_fd, WDIOC_SETTIMEOUT, timeout);
}


int print_last_boot_reason(const int boot_status)
{
    switch (boot_status)
    {
        case WDIOF_OVERHEAT:
            log_info("The last reboot was caused by the CPU overheating.");
            return 0;
        case WDIOF_CARDRESET:
            log_info("The last reboot was caused by a watchdog reset.");
            return 0;
        case WDIOF_FANFAULT:
            log_info("The last reboot was because a system fan monitored by the watchdog card failed.");
            return 0;
        case WDIOF_EXTERN1:
            log_info("The last reboot was because external monitoring relay/source 1 was triggered.");
            return 0;
        case WDIOF_EXTERN2:
            log_info("The last reboot was because external monitoring relay/source 2 was triggered.");
            return 0;
        case WDIOF_POWERUNDER:
            log_info("The last reboot was due to the machine showing an undervoltage status.");
            return 0;
        case WDIOF_POWEROVER:
            log_info("The last reboot was due to the machine showing an overvoltage status.");
            return 0;
    }

    return -1;
}


int get_last_boot_status(const int watchdog_fd)
{
    int flags;

    const int gs_ret = ioctl(watchdog_fd, WDIOC_GETBOOTSTATUS, &flags);
    if (is_err(gs_ret))
    {
        return gs_ret;
    }

    const int lb_ret = print_last_boot_reason(flags);
    if (is_err(lb_ret))
    {
        log_info("Last boot wasn't caused by the watchdog card");
    }

    return 0;
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
        {"device", required_argument, NULL, 'w'},
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
                log_warning("Only --timeout and --wdog-fname should be provided. Exiting.");
                return EXIT_FAILURE;
        }
    }

    const int wdog_fd = open(wdog_filename, O_WRONLY);
    if (is_err(wdog_fd))
    {
        return log_error("Failed to open watchdog device file");
    }

    const int gs_ret = get_last_boot_status(wdog_fd);
    if (is_err(gs_ret))
    {
        return log_error("Failed to get the last boot status");
    }

    const int to_ret = set_watchdog_timeout(wdog_fd, &timeout);
    if (is_err(to_ret))
    {
        return log_error("Failed to set the watchdog timeout");
    }

    const int cl_ret = close(wdog_fd);
    if (is_err(cl_ret))
    {
        return log_error("Failed to close the watchdog device file");
    }

    return EXIT_SUCCESS;
}
