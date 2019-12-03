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
#include "arm-handler-common.h"
#include "handler.h"
#include "swupdate.h"
#include "util.h"

int get_part_info_filepath(char *dst, const char *const file_name)
{
    static const char *const part_info_dir = "part-info";
    return snprintf(
            dst
            , strlen(FACTORY_CONFIG_PARTITION) + strlen(file_name) + strlen(part_info_dir) + 3
            ,  "%s/%s/%s"
            , FACTORY_CONFIG_PARTITION
            , part_info_dir
            , file_name);
}

char *read_part_info_file(const char *const file_name)
{
    char part_info_filepath[PATH_MAX];
    if (get_part_info_filepath(part_info_filepath, file_name) < 0)
    {
        ERROR("Failed to create part-info file path for %s", file_name);
        return NULL;
    }

    return read_file_to_new_str(part_info_filepath);
}

int copy_image_and_sync(struct img_type *img, const char *const device_filepath)
{
    int ret_val = 0;
    int fd = openfileoutput(device_filepath);
    if (fd < 0)
    {
        ERROR("%s%s", "Failed to open output file ", device_filepath);
        return -1;
    }

    if (copyimage(&fd, img, NULL) < 0)
    {
        ERROR("%s%s", "Failed to copy image to target device");
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
        ERROR("Failed to close file descriptor");
        ret_val = -1;
    }

    return ret_val;
}

int write_bootflags_file()
{
    if (mkdir(BOOTFLAGS_DIR, 0755) == -1)
    {
        if (errno != EEXIST)
        {
            ERROR("Failed to create %s: %s", BOOTFLAGS_DIR, strerror(errno));
            return -1;
        }
    }

    char bootflags_file_path[PATH_MAX];
    static const char *const fname = "/rootfs2";
    const size_t len = strlen(BOOTFLAGS_DIR) + strlen(fname) + 1;
    snprintf(bootflags_file_path, len, "%s%s", BOOTFLAGS_DIR, fname);

    FILE* file = fopen(bootflags_file_path, "w");
    if (!file)
    {
        return -1;
    }

    fprintf(file, "");
    fclose(file);

    return 0;
}

int rootfs_handler(struct img_type *img
                   , void __attribute__ ((__unused__)) *data)
{
    static const char *const root_mnt_point = "/";
    char mounted_device_filepath[PATH_MAX];
    if (get_mounted_device(mounted_device_filepath, root_mnt_point) != 0)
    {
        ERROR("%s", "Failed to get mounted device file path.");
        return 1;
    }

    char *b1_pnum = read_part_info_file("MBL_ROOT_FS_PART_NUMBER_BANK1");
    if (!b1_pnum)
    {
        ERROR("Failed to read file MBL_ROOT_FS_PART_NUMBER_BANK1");
        return 1;
    }

    int return_value = 0;

    char *b2_pnum = read_part_info_file("MBL_ROOT_FS_PART_NUMBER_BANK2");
    if (!b2_pnum)
    {
        ERROR("Failed to read file MBL_ROOT_FS_PART_NUMBER_BANK2");
        return_value = 1;
        goto free;
    }

    char target_device_filepath[PATH_MAX];
    if (find_target_partition(target_device_filepath, mounted_device_filepath, b1_pnum, b2_pnum) != 0)
    {
        ERROR("Failed to find target partition.");
        return_value = 1;
        goto free;
    }

    if (copy_image_and_sync(img, target_device_filepath) == -1)
    {
        ERROR("%s %s %s %s", "Failed to copy image", img->fname, "to device", target_device_filepath);
        return_value = 1;
        goto free;
    }

    if (str_endswith(target_device_filepath, b2_pnum))
    {
        if (write_bootflags_file() == -1)
        {
            ERROR("Failed to write boot flag file. Next boot will be from bank 1");
            return_value = 1;
            goto free;
        }
    }

free:
    str_delete(b1_pnum);
    str_delete(b2_pnum);
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

