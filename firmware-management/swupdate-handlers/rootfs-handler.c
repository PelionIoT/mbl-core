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


int get_target_device_file_path(char *target_device_filepath, const char *const mounted_device_filepath)
{
    int return_value = 0;

    char const bank1_pnum_filepath[PATH_MAX];
    char const bank2_pnum_filepath[PATH_MAX];
    if (get_part_info_filepath(bank1_pnum_filepath, "MBL_ROOT_FS_PART_NUMBER_BANK1") < 0)
    {
        ERROR("Failed to create part-info file path for MBL_ROOT_FS_PART_NUMBER_BANK1");
        return -1;
    }

    if (get_part_info_filepath(bank2_pnum_filepath,"MBL_ROOT_FS_PART_NUMBER_BANK2") < 0)
    {
        ERROR("Failed to create part-info file path for MBL_ROOT_FS_PART_NUMBER_BANK2");
        return -1;
    }

    char *b1_pnum;
    char *b2_pnum;
    if (!(b1_pnum = read_file_to_new_str(bank1_pnum_filepath)))
    {
        ERROR("%s%s", "Failed to read file: ", bank1_pnum_filepath);
        return_value = -1;
        goto free;
    }

    if (!(b2_pnum = read_file_to_new_str(bank2_pnum_filepath)))
    {
        ERROR("%s%s", "Failed to read file: ", bank2_pnum_filepath);
        return_value = -1;
        goto free;
    }

    if (find_target_bank(target_device_filepath, mounted_device_filepath, b1_pnum, b2_pnum) != 0)
    {
        ERROR("%s", "Failed to find target partition.");
        return_value = -1;
    }

free:
    str_delete(b1_pnum);
    str_delete(b2_pnum);
    return return_value;
}

int copy_image_to_block_device(struct img_type *img, const char *const device_filepath)
{
    int ret_val = 0;
    int fd = openfileoutput(device_filepath);
    if (fd < 0)
    {
        ERROR("%s%s", "Failed to open output file ", device_filepath);
        return -1;
    }

    const int ret = copyimage(&fd, img, NULL);
    if (ret < 0)
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

int write_bootflags_file(const char *const target_device)
{
    char const bank1_pnum_filepath[PATH_MAX];
    char const bank2_pnum_filepath[PATH_MAX];

    if (get_part_info_filepath(bank1_pnum_filepath, "MBL_ROOT_FS_PART_NUMBER_BANK1") < 0)
    {
        ERROR("Failed to create part-info file path for MBL_ROOT_FS_PART_NUMBER_BANK1");
        return -1;
    }

    if (get_part_info_filepath(bank2_pnum_filepath,"MBL_ROOT_FS_PART_NUMBER_BANK2") < 0)
    {
        ERROR("Failed to create part-info file path for MBL_ROOT_FS_PART_NUMBER_BANK2");
        return -1;
    }

    int ret_val = 0;
    char *b1_pnum;
    char *b2_pnum;
    if (!(b1_pnum = read_file_to_new_str(bank1_pnum_filepath)))
    {
        return -1;
    }

    if (!(b2_pnum = read_file_to_new_str(bank2_pnum_filepath)))
    {
        goto free;
        ret_val = -1;
    }

    if (mkdir(BOOTFLAGS_DIR, 0755) == -1)
    {
        if (errno != EEXIST)
        {
            ret_val = -1;
            goto free;
        }
    }

    if (str_endswith(b2_pnum, target_device) == 0)
    {
        char bootflags_file_path[PATH_MAX];
        snprintf(bootflags_file_path, BOOTFLAGS_DIR, "rootfs2");

        FILE* file;
        if (!(file = fopen(bootflags_file_path, "w")))
        {
            ret_val = -1;
            goto free;
        }

        if (fprintf(file, "") == -1)
        {
            ret_val = -1;
        }
        fclose(file);
    }

free:
    str_delete(b1_pnum);
    str_delete(b2_pnum);
    return ret_val;
}

int rootfs_handler(struct img_type *img
                   , void __attribute__ ((__unused__)) *data)
{
    static const char *const root_mnt_point = "/";
    char mounted_device_filepath[PATH_MAX];
    if (get_mounted_device_path(mounted_device_filepath, root_mnt_point) != 0)
    {
        ERROR("%s", "Failed to get mounted device file path.");
        return 1;
    }

    char target_device_filepath[PATH_MAX];
    if (get_target_device_file_path(target_device_filepath, mounted_device_filepath) != 0)
    {
        ERROR("%s", "Failed to get target device file path");
        return 1;
    }

    if (copy_image_to_block_device(img, target_device_filepath) == -1)
    {
        ERROR("%s %s %s %s", "Failed to copy image", img->fname, "to device", target_device_filepath);
        return 1;
    }

    if (write_bootflags_file(target_device_filepath) == -1)
    {
        ERROR("Failed to write boot flag file. Next boot will be from bank 1");
        return 1;
    }

    return 0;
}

__attribute__((constructor))
void rootfs_handler_init(void)
{
    register_handler("ROOTFSv4"
                     , rootfs_handler
                     , IMAGE_HANDLER
                     , NULL);
}

