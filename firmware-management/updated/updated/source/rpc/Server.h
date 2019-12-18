/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef UPDATED_RPC_SERVER_H
#define UPDATED_RPC_SERVER_H

#include <memory>

namespace grpc {
class Server;
class Service;
} // namespace grpc

namespace updated {
namespace rpc {

class Server final
{
public:
    Server();
    ~Server() noexcept;
    void shut_down();

private:
    const std::unique_ptr<grpc::Service> service_;
    const std::unique_ptr<grpc::Server> server_;
};

} // namespace rpc
} // namespace updated

#endif // UPDATED_RPC_SERVER_H
