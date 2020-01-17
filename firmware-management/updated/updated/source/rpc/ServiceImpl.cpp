/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ServiceImpl.h"
#include "../logging/logger.h"

#include <exception>

namespace updated {
namespace rpc {

grpc::Status ServiceImpl::GetUpdateHeader(
    grpc::ServerContext* const /* context */,
    const Empty* const /* request */,
    GetUpdateHeaderResponse* const response)
{
    response->set_update_header(update_coordinator.manifest().header);
    response->mutable_error_code()->set_value(ErrorCodeMessage::SUCCESS);
    return grpc::Status::OK;
}

grpc::Status ServiceImpl::StartUpdate(
    grpc::ServerContext* const /* context */,
    const StartUpdateRequest* const request,
    ErrorCodeMessage* const response)
{
    try
    {
        update_coordinator.start(request->payload_path(), request->update_header());
        response->set_value(ErrorCodeMessage::SUCCESS);
        return grpc::Status::OK;
    }
    catch(std::exception &e)
    {
        logging::error(e.what());
        response->set_value(ErrorCodeMessage::UNKNOWN_ERROR);
        return grpc::Status::CANCELLED;
    }
}

} // namespace rpc
} // namespace updated
