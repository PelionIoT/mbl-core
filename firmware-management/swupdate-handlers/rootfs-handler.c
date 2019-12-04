/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "swupdate.h"
#include "util.h"
#include "handler.h"
#include "arm-handler-common.h"

static int copy_image_and_sync(struct img_type *img, const char *const device_filepath)
{
    int ret_val = 0;
    int fd = openfileoutput(device_filepath);
    if (fd < 0)
    {
        ERROR("%s", "Failed to open target device file");
        return -1;
    }

    if (copyimage(&fd, img, NULL) < 0)
    {
        ERROR("%s", "Failed to copy image to target device");
        goto close;
        ret_val = -1;
    }

    if (fsync(fd) == -1)
    {
        WARN("%s: %s", "Failed to sync filesystem", strerror(errno));
    }

close:
    if (close(fd) == -1)
    {
        ERROR("%s", "Failed to close target device file descriptor");
        ret_val = -1;
    }

    return ret_val;
}

static int get_bootflag_file_path(char *const bootflags_file_path, const size_t size)
{
    static const char *const fname = "/rootfs2";
    int num_written = snprintf(bootflags_file_path, size, "%s%s", BOOTFLAGS_DIR, fname);
    if (num_written < 0)
    {
        ERROR("%s", "There was an output error when writing to the destination buffer");
        return -1;
    }

    if ((size_t)num_written >= size)
    {
        ERROR("%s", "bootflags filepath is larger than the destination buffer size");
        return -1;
    }

    return 0;
}

int write_bootflag_file()
{
    if (mkdir(BOOTFLAGS_DIR, 0755) == -1)
    {
        if (errno != EEXIST)
        {
            ERROR("%s %s: %s", "Failed to create", BOOTFLAGS_DIR, strerror(errno));
            return -1;
        }
    }

    char bootflags_file_path[PATH_MAX];
    if (get_bootflag_file_path(bootflags_file_path, PATH_MAX) < 1)
        return -1;

    FILE *const file = fopen(bootflags_file_path, "w");
    if (!file)
    {
        ERROR("%s: %s", "Failed to open bootflags file", strerror(errno));
        return -1;
    }

    fprintf(file, "%s", "");
    fclose(file);

    sync();
    return 0;
}

int remove_bootflag_file()
{
    char bootflags_file_path[PATH_MAX];
    if (get_bootflag_file_path(bootflags_file_path, PATH_MAX) < 0)
        return -1;

    if (remove(bootflags_file_path) == -1)
    {
        if (errno != ENOENT)
        {
            ERROR("%s: %s", "Failed to remove bootflags file", strerror(errno));
            return -1;
        }
    }

    sync();
    return 0;
}

int rootfs_handler(struct img_type *img
                   , void __attribute__ ((__unused__)) *data)
{
    static const char *const root_mnt_point = "/";
    static const int MAX_DEVICE_FILE_PATH = 512;
    char mounted_device_filepath[MAX_DEVICE_FILE_PATH];
    if (get_mounted_device(mounted_device_filepath, root_mnt_point, MAX_DEVICE_FILE_PATH) != 0)
    {
        ERROR("%s %s", "Failed to get mounted device file path", mounted_device_filepath);
        return 1;
    }

    char *b1_pnum = read_part_info_file_to_new_str("MBL_ROOT_FS_PART_NUMBER_BANK1");
    if (!b1_pnum)
    {
        ERROR("%s", "Failed to read file MBL_ROOT_FS_PART_NUMBER_BANK1");
        return 1;
    }

    int return_value = 0;

    char *b2_pnum = read_part_info_file_to_new_str("MBL_ROOT_FS_PART_NUMBER_BANK2");
    if (!b2_pnum)
    {
        ERROR("%s", "Failed to read file MBL_ROOT_FS_PART_NUMBER_BANK2");
        return_value = 1;
        goto free;
    }

    char target_device_filepath[MAX_DEVICE_FILE_PATH];
    if (find_target_partition(target_device_filepath, mounted_device_filepath, b1_pnum, b2_pnum, MAX_DEVICE_FILE_PATH) != 0)
    {
        ERROR("%s %s", "Failed to find target partition", target_device_filepath);
        return_value = 1;
        goto free;
    }

    if (copy_image_and_sync(img, target_device_filepath) == -1)
    {
        ERROR("%s %s %s", "Failed to copy image", img->fname, "to target partition");
        return_value = 1;
        goto free;
    }

    if (str_endswith(target_device_filepath, b2_pnum) == 0)
    {
        if (write_bootflag_file() == -1)
        {
            ERROR("%s", "Failed to write boot flag file. Next boot will be from bank 1");
            return_value = 1;
            goto free;
        }
    }
    else
    {
        if (remove_bootflag_file() == -1)
        {
            ERROR("%s", "Failed to remove bootflag file.");
            return_value = 1;
        }
    }

free:
    free(b1_pnum);
    free(b2_pnum);
    return return_value;
}

__attribute__((constructor))
void rootfs_handler_init(void)
{
    register_handler("ROOTFSv4"
                     , rootfs_handler
                     , IMAGE_HANDLER
                     , NULL);
}
