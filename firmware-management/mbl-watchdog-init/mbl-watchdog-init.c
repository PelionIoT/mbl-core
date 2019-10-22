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
#include <limits.h>
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

// Uses errno to determine the last error code.
// Must be called immediately after the call which could throw an error
// so errno doesn't change.
void log_error(const char * const message)
{
    static const char * const prefix = "WATCHDOG-ERROR:";
    const int error_num = errno;
    const char* const err_msg = strerror(error_num);

    fprintf(stderr, "%s %s %s\n", prefix, message, err_msg);
}


void log_warning(const char * const message)
{
    static const char * const prefix = "WATCHDOG-WARNING:";
    fprintf(stderr, "%s %s\n", prefix, message);
}


void log_info(const char * const message)
{
    static const char * const prefix = "WATCHDOG-INFO:";
    fprintf(stdout, "%s %s\n", prefix, message);
}

//============================================================================
// Watchdog functions
int set_watchdog_timeout(const int watchdog_fd, int * const timeout)
{
    return ioctl(watchdog_fd, WDIOC_SETTIMEOUT, timeout);
}


void print_last_boot_reason(const int boot_status)
{
    if (boot_status == 0)
    {
        log_info("Normal boot.");
    }
    else
    {
        if (boot_status & WDIOF_OVERHEAT)
            log_warning("The last reboot was caused by the CPU overheating.");

        if (boot_status & WDIOF_CARDRESET)
            log_warning("The last reboot was caused by a watchdog reset.");

        if (boot_status & WDIOF_FANFAULT)
            log_warning("The last reboot was because a system fan monitored by the watchdog card failed.");

        if (boot_status & WDIOF_EXTERN1)
            log_warning("The last reboot was because external monitoring relay/source 1 was triggered.");

        if (boot_status & WDIOF_EXTERN2)
            log_warning("The last reboot was because external monitoring relay/source 2 was triggered.");

        if (boot_status & WDIOF_POWERUNDER)
            log_warning("The last reboot was due to the machine showing an undervoltage status.");

        if (boot_status & WDIOF_POWEROVER)
            log_warning("The last reboot was due to the machine showing an overvoltage status.");
    }
}


int get_last_boot_status(const int watchdog_fd)
{
    int flags;

    const int gs_ret = ioctl(watchdog_fd, WDIOC_GETBOOTSTATUS, &flags);
    if (is_err(gs_ret))
    {
        return gs_ret;
    }

    print_last_boot_reason(flags);

    return 0;
}


//============================================================================
// Convert a string containing only digits to an integer. Exits on errors.
int numeric_string_to_positive_int(const char* const str)
{
    errno = 0;
    char *endptr;
    const long int res = strtol(str, &endptr, 0);

    if ((errno == ERANGE && (res == LONG_MAX || res == LONG_MIN))
           || (errno != 0 && res == 0))
    {
        log_error("strtol failed");
        exit(EXIT_FAILURE);
    }

    if (res <= 0 || res > INT_MAX)
    {
        log_error("Integer not in valid range. Expecting a positive integer with maximum size INT_MAX");
        exit(EXIT_FAILURE);
    }

    if (endptr == str)
    {
        log_error("String must contain digits only.");
        exit(EXIT_FAILURE);
    }

    const int num = res;

    return num;
}

//============================================================================
// Main entry point
int main(int argc, char **argv)
{
    int timeout = 0;
    const char* wdog_filename = "/dev/watchdog";

    // Parse command line arguments
    int current_opt;
    int optindex;
    static const struct option longopts[] = {
        {"timeout", required_argument, NULL, 't'},
        {"device", required_argument, NULL, 'w'},
    };

    while ((current_opt = getopt_long(argc, argv, "tw:", longopts, &optindex)) != -1)
    {
        switch (current_opt)
        {
            case 't':
                timeout = numeric_string_to_positive_int(optarg);
                break;
            case 'w':
                wdog_filename = optarg;
                break;
            default:
                log_warning("Only --timeout and --device should be provided. Exiting.");
                return EXIT_FAILURE;
        }
    }

    if (timeout <= 0)
    {
        log_error("--timeout must be passed and it must be a positive integer");
        return EXIT_FAILURE;
    }

    const int wdog_fd = open(wdog_filename, O_WRONLY);
    if (is_err(wdog_fd))
    {
        log_error("Failed to open watchdog device file");
        return EXIT_FAILURE;
    }

    // Set return value to EXIT_SUCCESS, will be set to EXIT_FAILURE if any of
    // the following calls fail.
    int ret = EXIT_SUCCESS;

    const int gs_ret = get_last_boot_status(wdog_fd);
    if (is_err(gs_ret))
    {
        log_error("Failed to get the last boot status");
        ret = EXIT_FAILURE;
        goto close;
    }

    const int to_ret = set_watchdog_timeout(wdog_fd, &timeout);
    if (is_err(to_ret))
    {
        log_error("Failed to set the watchdog timeout");
        ret = EXIT_FAILURE;
        goto close;
    }

close:
    if (is_err(close(wdog_fd)))
    {
        log_error("Failed to close the watchdog device file");
        ret = EXIT_FAILURE;
    }

    return ret;
}
