/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef UPDATED_RPC_SERVER_H
#define UPDATED_RPC_SERVER_H

#include "../UpdateCoordinator.h"

#include <grpc++/server.h>

#include <memory>

namespace updated {
namespace rpc {

/**
 * Class for UpdateD RPC servers.
 *
 * A server is started when a Server object is created and is shut down when
 * the object is destroyed.
 *
 * The server listens for RPC requests and services them using an
 * updated::rpc::ServiceImpl object.
 */
class Server final
{
public:
    /**
     * Create an updated::rpc::Server.
     *
     * This includes setting up a socket for listening and starting a new
     * thread (or threads) for servicing RPC requests.
     */
    Server(updated::UpdateCoordinator&);

    /**
     * Shut down the RPC server and wait for in-progress RPCs to finish.
     */
    ~Server() noexcept;

    /**
     * Asynchronously send a shutdown message to the RPC server.
     */
    void shut_down();

private:
    const std::unique_ptr<grpc::Service> service_;
    const std::unique_ptr<grpc::Server> server_;
};

} // namespace rpc
} // namespace updated

#endif // UPDATED_RPC_SERVER_H
