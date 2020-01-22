/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LockFile.h"
#include "../logging/logger.h"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace updated {
namespace fileutils {

LockFile::LockFile(const std::filesystem::path &path)
{
    fd = open(path.string().c_str(), O_RDWR | O_CREAT, S_IRWXU); // NOLINT
    if (fd == -1)
    {
        logging::error("Failed to open the lock file at path {}", path.string());
        throw LockFileError{errno};
    }

    const auto lockf_ret = lockf(fd, F_TLOCK, 0); // NOLINT
    if (lockf_ret == -1)
    {
        logging::error("Failed to obtain a lock on the lock file. Path {}, FD {}", path.string(), fd);
        throw LockFileError{errno};
    }
}

LockFile::~LockFile()
{
    if (close(fd) == -1)
    {
    }
}

} // namespace fileutils
} // namespace updated
