/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_MANIFEST_H
#define UPDATED_MANIFEST_H

#include <string>

namespace updated {

/** Manifest contains information about an update bundle.
 *  Currently this object just contains the data from the update HEADER file.
 */
struct Manifest final
{
    std::string header;
};

} // namespace updated

#endif // UPDATED_MANIFEST_H
