/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_UPDATE_COORDINATOR_H
#define UPDATED_UPDATE_COORDINATOR_H

#include "Manifest.h"
#include "fileutils/payload.h"

#include <cassert>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>

namespace updated {

/**
 * Coordinate asynchronous updates.
 *
 * UpdateCoordinator is the core object responsible for updates.
 * After instantiating the UpdateCoordinator you must call the run method,
 * which will block the current thread until the start method is called.
 * This design was chosen to allow UpdateD's RPC server to trigger an update
 * asynchronously.
 */
class UpdateCoordinator final
{
public:
    UpdateCoordinator() = default;

    /**
     * Start an update.
     *
     * This method is ALWAYS called on a different thread to UpdateCoordinator::run
     */
    void start(const std::filesystem::path &payload_path, std::string header_data) noexcept;

    /**
     * Run an update transaction.
     *
     * Block the main thread and wait until an update request arrives.
     * When the request arrives delegate the update to a component installer.
     */
    void run();

    /** Return the update manifest */
    Manifest manifest() const noexcept;

private:
    std::mutex mutex;
    std::condition_variable condition_var;
    // NOTE: this is a temporary flag until we can query our global update state
    // from UpdateTracker
    bool updating{false};

    std::unique_ptr<fileutils::PayloadHardLink> payload_link;
    Manifest update_manifest;
};

} // namespace updated

#endif // UPDATED_UPDATE_COORDINATOR_H
