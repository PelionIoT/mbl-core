/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Server.h"
#include "ServiceImpl.h"

#include "updated-rpc/config.h"
#include "updated-rpc/Error.h"

#include <grpc++/server.h>
#include <grpc++/server_builder.h>

#include <cassert>

static const char* const g_listen_addr = "0.0.0.0:" UPDATED_RPC_DEFAULT_PORT_STR;

namespace updated {
namespace rpc {

namespace {

std::unique_ptr<grpc::Server> create_grpc_server(grpc::Service* const service)
{
    assert(service);

    std::unique_ptr<grpc::Server> server = grpc::ServerBuilder()
        .AddListeningPort(
            g_listen_addr,
            grpc::InsecureServerCredentials()
        )
        .RegisterService(service)
        .BuildAndStart();

    if (!server)
    {
        throw Error("Failed to build and start gRPC server.");
    }
    return server;
}

} // anonymous namespace

Server::Server()
    : service_{std::make_unique<ServiceImpl>()}
    , server_{create_grpc_server(service_.get())}
{
}

Server::~Server() noexcept
{
    server_->Shutdown();
    server_->Wait();
}

void Server::shut_down()
{
    server_->Shutdown();
}

} // namespace rpc
} // namespace updated
