/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Client.h"

#include "updated-rpc/config.h"
#include "updated-rpc/Error.h"

#include <grpc++/create_channel.h>

static const char* const g_server_addr = "localhost:" UPDATED_RPC_DEFAULT_PORT_STR;

namespace updated {
namespace rpc {

namespace {

std::unique_ptr<UpdateDService::Stub> create_grpc_service_stub()
{
    auto channel = grpc::CreateChannel(g_server_addr, grpc::InsecureChannelCredentials());
    if (!channel) {
        throw Error("Failed to create gRPC channel");
    }

    auto service_stub = rpc::UpdateDService::NewStub(channel);
    if (!service_stub) {
        throw Error("Failed to create gRPC service");
    }
    return service_stub;
}

} // anonymous namespace

Client::Client()
    : service_stub_(create_grpc_service_stub())
{
}

std::string Client::GetUpdateHeader()
{
    GetUpdateHeaderResponse response;
    grpc::ClientContext context;
    const auto rpc_status = service_stub_->GetUpdateHeader(&context, Empty(), &response);
    throw_on_grpc_error(rpc_status);
    throw_on_updated_rpc_error(response.error_code());
    return response.update_header();
}

void Client::StartUpdate(std::string_view payload_path, std::string_view update_header)
{
    grpc::ClientContext context;
    ErrorCodeMessage response;
    StartUpdateRequest request;
    request.set_payload_path(payload_path.data(), payload_path.size());
    request.set_update_header(update_header.data(), update_header.size());
    const auto rpc_status = service_stub_->StartUpdate(&context, &request, &response);
    throw_on_grpc_error(rpc_status);
    throw_on_updated_rpc_error(response);
}

} // namespace rpc
} // namespace updated
