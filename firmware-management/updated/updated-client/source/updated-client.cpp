/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpc/Client.h"

int main()
{
    updated::rpc::Client client;
    std::cout << client.GetUpdateHeader() << '\n';
    return 0;
}
