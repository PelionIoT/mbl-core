/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpc/Client.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/**
 * Parse command line arguments.
 */
std::pair<std::string, std::string> parse_args(const int argc, char **argv)
{
    int current_opt;
    int optindex;

    std::vector<option> long_opts {
        {"payload-filepath", required_argument, 0, 'p'},
        {"update-header-filepath", required_argument, 0, 'u'}
    }

    std::pair<std::string, std::string> arg_values;
    while ((current_opt = getopt_long(argc, argv, "p:u:", long_opts.data(), &optindex)) != -1)
    {
        switch (current_opt)
        {
            case 'p':
                arg_values.first = optarg;
                break;
            case 'u':
                arg_values.second = optarg;
                break;
            case '?':
                throw std::invalid_argument("Unrecognized argument!");
            default:
                break;
        }
    }

    if (!arg_values.first.size())
        throw std::invalid_argument("Must provide payload file path!");
    if (!arg_values.second.size())
        throw std::invalid_argument("Must provide HEADER file path!");

    return arg_values;
}

int main(int argc, char **argv)
{
    const auto [payload_path, header_data] = parse_args(argc, argv);
    updated::rpc::Client client;
    client.StartUpdate(payload_path, header_data);
    return 0;
}
