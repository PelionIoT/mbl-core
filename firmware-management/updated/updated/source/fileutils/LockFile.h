/*
 * Copyright (c) 2020 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATED_LOCKFILE_H
#define UPDATED_LOCKFILE_H

#include <filesystem>
#include <system_error>

namespace updated {
namespace fileutils {

class LockFileError final
    : public std::system_error
{
public:
   explicit LockFileError(int errno_code)
       : system_error(std::make_error_code(static_cast<std::errc>(errno_code)))
   {}
};

class LockFile final
{
public:
    explicit LockFile(const std::filesystem::path &lockfile_path);
    ~LockFile();

private:
    int fd;
};

} // namespace fileutils
} // namespace updated

#endif // UPDATED_LOCKFILE_H
