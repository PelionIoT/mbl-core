/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef UPDATED_RPC_SERVICEIMPL_H
#define UPDATED_RPC_SERVICEIMPL_H

#include "updated-rpc/updated-rpc.grpc.pb.h"

namespace updated {
namespace rpc {

class ServiceImpl final
    : public UpdateDService::Service
{
public:
    ServiceImpl() = default;

private:
    grpc::Status GetUpdateHeader(
        grpc::ServerContext*,
        const Empty*,
        GetUpdateHeaderResponse* response
    ) override;

    grpc::Status StartUpdate(
        grpc::ServerContext*,
        const StartUpdateRequest* request,
        ErrorCodeMessage* response
    ) override;
};

} // namespace rpc
} // namespace updated

#endif // UPDATED_RPC_SERVICEIMPL_H
