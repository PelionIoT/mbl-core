/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "arm-handler-common.h"
#include "rootfs-handler.h"
#include "swupdate/swupdate.h"
#include "swupdate/util.h"
#include "swupdate/handler.h"

// WARNING: if you add new handlers you need to register them in arm-handlers.c
// added to the swupdate build by swupdate_%.bb in the meta-mbl repo.

int rootfsv4_handler(struct img_type *img, void __attribute__ ((__unused__)) *data)
{
    static const size_t MAX_DEVICE_FILE_PATH = 512;
    static const char *const root_mnt_point = "/";

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

    char *b2_pnum = read_part_info_file_to_new_str("MBL_ROOT_FS_PART_NUMBER_BANK2");
    if (!b2_pnum)
    {
        ERROR("%s", "Failed to read file MBL_ROOT_FS_PART_NUMBER_BANK2");
        free(b1_pnum);
        return 1;
    }

    int return_value = 0;
    char target_device_filepath[MAX_DEVICE_FILE_PATH];
    if (find_target_partition(target_device_filepath, mounted_device_filepath, b1_pnum, b2_pnum, MAX_DEVICE_FILE_PATH) != 0)
    {
        ERROR("%s %s", "Failed to find target partition", target_device_filepath);
        return_value = 1;
        goto clean;
    }

    if (copy_image_and_sync(img, target_device_filepath) == -1)
    {
        ERROR("%s %s %s", "Failed to copy image", img->fname, "to target partition");
        return_value = 1;
        goto clean;
    }

    static const char *const rootfs_filename = "rootfs2";
    if (str_endswith(b2_pnum, target_device_filepath) == 0)
    {
        if (write_bootflag_file(rootfs_filename) == -1)
        {
            ERROR("%s", "Failed to write bootflag file. Next boot will be from bank 1");
            return_value = 1;
            goto clean;
        }
    }
    else
    {
        if (remove_bootflag_file(rootfs_filename) == -1)
        {
            ERROR("%s", "Failed to remove bootflag file.");
            return_value = 1;
        }
    }

clean:
    free(b1_pnum);
    free(b2_pnum);
    return return_value;
}
