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
    fd = open(path.string().c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IRWXU); // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    if (fd == -1)
    {
        logging::error("Failed to open the lock file at path {}", path.string());
        throw LockFileError{errno};
    }

    const auto lockf_ret = lockf(fd, F_TLOCK, 0);
    if (lockf_ret == -1)
    {
        logging::error("Failed to obtain a lock on the lock file. Path {}, FD {}", path.string(), fd);
        throw LockFileError{errno};
    }
}

LockFile::~LockFile()
{
    close(fd);
}

} // namespace fileutils
} // namespace updated
