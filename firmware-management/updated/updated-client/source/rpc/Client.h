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

/**
 * A client for the UpdateD RPC mechanism.
 */
class Client
{
public:
    /**
     * Create an updated::rpc::Client. 
     *
     * Creating a Client object. This includes setting up a channel through
     * which RPCs can be made by calling the object's member functions.
     */
    Client();

    /**
     * Get the update HEADER file for the latest successful firmware update.
     *
     * UpdateD treats HEADER files as opaque blobs that are associated with
     * each update transaction (something like update transaction handles).
     */
    std::string GetUpdateHeader();

private:
    std::unique_ptr<UpdateDService::Stub> service_stub_;
};

} // namespace rpc 
} // namespace updated

#endif // UPDATED_CLIENT_RPC_CLIENT_H
