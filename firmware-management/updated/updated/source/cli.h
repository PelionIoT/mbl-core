/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_CLI_H
#define UPDATED_CLI_H

#include <cstring>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#include <getopt.h>

namespace updated {
namespace cli {

/**
 * Return help text.
 */
const char* usage()
{
    return R"(Usage: updated [-l] CRITICAL|ERROR|WARNING|INFO|DEBUG|TRACE
UpdateD, a system daemon that coordinates firmware updates.
Example: updated -l CRITICAL

Options:
    -l          Set the logging level (default INFO). Possible values: CRITICAL|ERROR|WARNING|INFO|DEBUG|TRACE
)";
}

/**
 * Parse command line arguments.
 */
std::string parse_args(const int argc, char **argv)
{
    if (argc < 2)
    {
        return "INFO";
    }

    int current_opt;
    int optindex;

    static const std::vector<option> long_opts {
        {"log-level", optional_argument, nullptr, 'l'},
        {"help", no_argument, nullptr, 'h'}
    };

    while ((current_opt = getopt_long(argc, argv, "l:h", long_opts.data(), &optindex)) != -1)
    {
        switch (current_opt)
        {
            case 'l':
                if (!std::strcmp(optarg, "CRITICAL") || !std::strcmp(optarg, "ERROR")
                        || !std::strcmp(optarg, "WARNING") || !std::strcmp(optarg, "INFO")
                        || !std::strcmp(optarg, "DEBUG") || !std::strcmp(optarg, "TRACE"))
                {
                    return optarg;
                }
                else
                {
                    std::cout << usage() << '\n';
                    throw std::invalid_argument("Invalid log level given!");
                }
            case 'h':
                std::cout << usage() << '\n';
                std::exit(0);
            case '?':
                std::cout << usage() << '\n';
                throw std::invalid_argument("Unrecognized argument!");
            default:
                break;
        }
    }

    std::cout << usage() << '\n';
    throw std::invalid_argument("Unrecognized arguments!");
}

} // namespace cli
} // namespace updated

#endif // UPDATED_CLI_H
