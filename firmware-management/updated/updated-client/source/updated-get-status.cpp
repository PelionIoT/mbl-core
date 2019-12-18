/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpc/Client.h"


int main()
{
    updated::rpc::Client client;
    const auto header = client.GetUpdateHeader();
    if (header.empty())
    {
        std::cerr << "UpdateD returned an empty update HEADER.\n";
        return 1;
    }

    std::cout << header << '\n';
    return 0;
}
