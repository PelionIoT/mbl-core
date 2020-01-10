/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "UpdateCoordinator.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace updated {

void UpdateCoordinator::start(const std::filesystem::path &payload_path, const std::string header_data) noexcept //NOLINT: Allow a value param here for concurrency reasons
{
    assert((!payload_path.empty())); //NOLINT:
    assert((!header_data.empty())); //NOLINT:

    std::unique_lock<std::mutex> ul{ mutex };
    update_manifest.header = header_data;
    payload_link = std::make_unique<fileutils::PayloadHardLink>(payload_path);
    //TODO: set global update state to UPDATING instead of using this flag
    updating = true;
    std::cout << "starting update: updating flag = " << updating << '\n';
    // manually unlock the mutex before notifying waiting threads.
    ul.unlock();
    condition_var.notify_all();
}

void UpdateCoordinator::run()
{
    std::unique_lock<std::mutex> ul{ mutex };
    // TODO: query UpdateTracker state to guard against spurious wakeups
    // instead of using this flag
    std::cout << "run thread waiting: updating flag = " << updating << '\n';
    condition_var.wait(ul, [this](){ return updating; });
    // TODO: call swupdate as a subprocess and block until it completes
    std::cout << "run thread wakeup: updating flag = " << updating << '\n';
    assert(payload_link != nullptr);//NOLINT
    std::cout << "call swupdate with payload " << payload_link->get() << '\n';
    updating = false;
    std::cout << "Removing payload hard link" << '\n';
    payload_link.reset(nullptr);
}

Manifest UpdateCoordinator::manifest() const noexcept
{
    return update_manifest;
}

} // namespace updated
