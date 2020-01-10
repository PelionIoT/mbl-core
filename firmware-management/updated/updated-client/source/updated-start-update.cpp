/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpc/Client.h"

#include <getopt.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

/**
 * Return help text.
 */
const char* usage()
{
    return R"(Usage: updated-start-update [-p] PATH [-u] HEADER_DATA
Send an RPC to UpdateD, passing the update payload file and header data.
Example: updated-start-update -p /tmp/payload.swu -u $(cat /tmp/update-header))";
}

/**
 * Parse command line arguments.
 */
std::pair<std::string, std::string> parse_args(const int argc, char **argv)
{
    if (argc < 2)
    {
        throw std::invalid_argument("No arguments given!");
    }

    int current_opt;
    int optindex;

    static const std::vector<option> long_opts {
        {"payload-filepath", required_argument, nullptr, 'p'},
        {"update-header", required_argument, nullptr, 'u'},
        {"help", no_argument, nullptr, 'h'}
    };

    std::pair<std::string, std::string> arg_values;
    while ((current_opt = getopt_long(argc, argv, "p:u:h", long_opts.data(), &optindex)) != -1)
    {
        switch (current_opt)
        {
            case 'p':
                arg_values.first = optarg;
                break;
            case 'u':
                arg_values.second = optarg;
                break;
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

    if (arg_values.first.empty() || !std::filesystem::exists(arg_values.first))
    {
        throw std::invalid_argument("Must provide a valid path to an update payload!");
    }

    if (arg_values.second.empty())
    {
        throw std::invalid_argument("Must provide HEADER data!");
    }

    return arg_values;
}

} // anonymous namespace

int main(int argc, char **argv)
{
    try
    {
        const auto [payload_path, header_data] = parse_args(argc, argv);
        updated::rpc::Client client;
        client.StartUpdate(payload_path, header_data);
    }
    catch(std::invalid_argument &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
