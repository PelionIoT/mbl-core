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

const char* grpc_status_to_str(const grpc::Status& grpc_status)
{
    switch (grpc_status.error_code()) {
        case grpc::OK: return "grpc::OK";
        case grpc::CANCELLED: return "grpc::CANCELLED";
        case grpc::UNKNOWN: return "grpc::UNKNOWN";
        case grpc::DEADLINE_EXCEEDED: return "grpc::DEADLINE_EXCEEDED";
        case grpc::UNAUTHENTICATED: return "grpc::UNAUTHENTICATED";
        case grpc::RESOURCE_EXHAUSTED: return "grpc::RESOURCE_EXHAUSTED";
        case grpc::UNIMPLEMENTED: return "grpc::UNIMPLEMENTED";
        case grpc::INTERNAL: return "grpc::INTERNAL";
        case grpc::UNAVAILABLE: return "grpc::UNAVAILABLE";
        case grpc::INVALID_ARGUMENT: return "grpc::INVALID_ARGUMENT";
        case grpc::NOT_FOUND: return "grpc::NOT_FOUND";
        case grpc::ALREADY_EXISTS: return "grpc::ALREADY_EXISTS";
        case grpc::PERMISSION_DENIED: return "grpc::PERMISSION_DENIED";
        case grpc::FAILED_PRECONDITION: return "grpc::FAILED_PRECONDITION";
        case grpc::ABORTED: return "grpc::ABORTED";
        case grpc::OUT_OF_RANGE: return "grpc::OUT_OF_RANGE";
        case grpc::DATA_LOSS: return "grpc::DATA_LOSS";
        case grpc::DO_NOT_USE: return "grpc::DO_NOT_USE";
    }
    return "<UNRECOGNIZED GRPC STATUS>";
}

void throw_on_grpc_error(const grpc::Status& grpc_status)
{
    if (!grpc_status.ok()) 
    {
        throw Error(grpc_status_to_str(grpc_status));
    }
}

const char* updated_rpc_error_code_message_to_str(const ErrorCodeMessage& error_code_message)
{
    switch(error_code_message.value())
    {
        case ErrorCodeMessage::UNKNOWN_ERROR: return "UNKOWN_ERROR";
        case ErrorCodeMessage::SUCCESS: return "SUCCESS";

        // protobuff generated enums have some extra values with names that
        // should really stay internal to protobuf. Adding cases for them here
        // means that the compiler can check that we've handled all cases
        // though...
        case ErrorCodeMessage_ErrorCode_ErrorCodeMessage_ErrorCode_INT_MIN_SENTINEL_DO_NOT_USE_:
        case ErrorCodeMessage_ErrorCode_ErrorCodeMessage_ErrorCode_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
    }
    return "<UNRECOGNIZED UPDATED RPC ERROR>";
}

void throw_on_updated_rpc_error(const ErrorCodeMessage& error_code_message)
{
    if (error_code_message.value() != ErrorCodeMessage::SUCCESS)
    {
        throw Error(updated_rpc_error_code_message_to_str(error_code_message));
    }
}

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

} // namespace rpc
} // namespace updated
