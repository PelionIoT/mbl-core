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

/**
 * Implements the UpdateD RPC service. Methods should only be called from gRPC.
 */
class ServiceImpl final
    : public UpdateDService::Service
{
public:
    ServiceImpl() = default;

private:
    /**
     * Implement the GetUpdateHeader RPC.
     *
     * This RPC returns the contents of the update HEADER file for the most
     * recent successful firmware update transaction.
     */
    grpc::Status GetUpdateHeader(
        grpc::ServerContext*,
        const Empty*,
        GetUpdateHeaderResponse* response
    ) override;

    /**
     * Implement the StartUpdate RPC.
     *
     * This RPC asks UpdateD to start a new update transaction.
     */
    grpc::Status StartUpdate(
        grpc::ServerContext*,
        const StartUpdateRequest* request,
        ErrorCodeMessage* response
    ) override;
};

} // namespace rpc
} // namespace updated

#endif // UPDATED_RPC_SERVICEIMPL_H
