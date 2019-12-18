/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_CLIENT_RPC_CLIENT_H
#define UPDATED_CLIENT_RPC_CLIENT_H

#include "updated-rpc/updated-rpc.grpc.pb.h"

#include <memory>
#include <string>

namespace updated {
namespace rpc {

class Client
{
public:
    Client();
    std::string GetUpdateHeader();

private:
    std::unique_ptr<UpdateDService::Stub> service_stub_;
};

} // namespace rpc 
} // namespace updated

#endif // UPDATED_CLIENT_RPC_CLIENT_H
