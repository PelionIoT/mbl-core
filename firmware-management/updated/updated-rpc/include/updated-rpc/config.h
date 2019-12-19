/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef UPDATED_RPC_CONFIG_H
#define UPDATED_RPC_CONFIG_H

// Just a helper to turn things into string literals
#define UPDATED_RPC_STRINGIFY(x) UPDATED_RPC_STRINGIFY_HELPER(x)
#define UPDATED_RPC_STRINGIFY_HELPER(x) #x

// The default UpdateD RPC port number
#define UPDATED_RPC_DEFAULT_PORT 50051

// The default UpdateD RPC port number as a string literal
#define UPDATED_RPC_DEFAULT_PORT_STR UPDATED_RPC_STRINGIFY(UPDATED_RPC_DEFAULT_PORT)

#endif // UPDATED_RPC_CONFIG_H
