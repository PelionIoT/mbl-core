/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_PAYLOAD_H
#define UPDATED_PAYLOAD_H

#include <cassert>
#include <filesystem>

namespace updated {
namespace fileutils {

/**
 * Manage a hard link to an update payload
 *
 * This object will create the hard link in a temporary directory, and ensure
 * it is cleaned up in an exception safe way.
 * If the temporary directory already exists, it and its contents are removed
 * before creating the hard link.
 */
class PayloadHardLink final
{
public:
    explicit PayloadHardLink(const std::filesystem::path &source_path)
    {
        assert(source_path.is_absolute());

        // Create a temporary directory in which to store the hard link.
        // it is important that the hard link remains on the same fs as
        // the source. filesystem::create_hard_link will fail to create
        // links across filesystem boundaries.
        // We also remove the directory first if it already exists.
        payload_path /= source_path.root_path()
                     /= source_path.relative_path().remove_filename();
        payload_path /= "updated";
        if (std::filesystem::exists(payload_path))
            std::filesystem::remove_all(payload_path);
        std::filesystem::create_directories(payload_path);
        // now create the hard link in the temporary dir
        payload_path /= source_path.filename();
        std::filesystem::create_hard_link(source_path, payload_path);
    }

    // non-copyable and non-movable
    PayloadHardLink(const PayloadHardLink&) = delete;
    PayloadHardLink(const PayloadHardLink&&) = delete;
    PayloadHardLink& operator=(const PayloadHardLink&) = delete;
    PayloadHardLink& operator=(const PayloadHardLink&&) = delete;

    ~PayloadHardLink() noexcept
    {
        if (std::filesystem::exists(payload_path.parent_path()))
            std::filesystem::remove_all(payload_path.remove_filename());
    }

    std::filesystem::path get() const noexcept
    {
        return payload_path;
    }

private:
    std::filesystem::path payload_path;
};

} // namespace fileutils
} // namespace updated

#endif // UPDATED_PAYLOAD_H
