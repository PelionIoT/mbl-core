/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ServiceImpl.h"

namespace updated {
namespace rpc {

grpc::Status ServiceImpl::GetUpdateHeader(
    grpc::ServerContext* const /* context */,
    const Empty* const /* request */,
    GetUpdateHeaderResponse* const response)
{
    // TODO: return actual update HEADER data, not this dummy data.
    response->set_update_header("asdf");
    response->mutable_error_code()->set_value(ErrorCodeMessage::SUCCESS);
    return grpc::Status::OK;
}

grpc::Status ServiceImpl::StartUpdate(
    grpc::ServerContext* const /* context */,
    const StartUpdateRequest* const /* request */,
    ErrorCodeMessage* const response)
{
    // TODO: actually ask UpdateCoordinator to start an update.
    response->set_value(ErrorCodeMessage::SUCCESS);
    return grpc::Status::OK;
}

} // namespace rpc
} // namespace updated
