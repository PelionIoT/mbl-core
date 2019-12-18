/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef UPDATED_UPDATEDRPC_ERROR_H
#define UPDATED_UPDATEDRPC_ERROR_H

#include <stdexcept>

namespace updated {
namespace rpc {

class Error
    : public std::runtime_error
{
    // "inherit" std::runtime_error's ctors
    using std::runtime_error::runtime_error;
};

} // namespace rpc
} // namespace updated

#endif // UPDATED_UPDATEDRPC_ERROR_H
