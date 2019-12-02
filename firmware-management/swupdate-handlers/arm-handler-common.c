/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <mntent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "util.h"
#include <arm-handler-common.h>

char *str_new(const size_t size)
{
    char *dst;
    if (!(dst = malloc(sizeof dst * size)))
    {
        ERROR("%s", "Failed to allocate string");
        return NULL;
    }
    return dst;
}

void str_delete(char *str)
{
    free(str);
}

char *str_copy(const char *src)
{
    if (!src)
        return NULL;

    char *dst = str_new(strlen(src) + 1);
    if (!dst)
    {
        return NULL;
    }

    if (!strncpy(dst, src, strlen(src)))
    {
        ERROR("%s", "Failed to copy str");
        str_delete(dst);
        return NULL;
    }

    if (dst[strlen(src)] != '\0')
        dst[strlen(src)] = '\0';

    return dst;
}

int str_endswith(const char *const substr, const char *const fullstr)
{
    const size_t endlen = strlen(fullstr) - strlen(substr);
    if (endlen <= 0)
        return 1;
    return strncmp(&fullstr[endlen], substr, strlen(substr));
}

char *read_file_to_new_str(const char *const filepath)
{
    char *return_val;
    struct stat stat_buf;
    if (stat(filepath, &stat_buf) == -1)
    {
        ERROR("%s %s: %s", "Failed to stat file", filepath, strerror(errno));
        return NULL;
    }

    char *dst = str_new(stat_buf.st_size + 1);
    if (!dst)
    {
        ERROR("%s", "Failed to allocate string buffer");
        return NULL;
    }

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        ERROR("%s: %s", "Failed to open file", strerror(errno));
        return_val = NULL;
        goto clean;
    }

    fread(dst, 1, stat_buf.st_size, fp);
    if (feof(fp) != 0 || ferror(fp) != 0)
    {
        ERROR("%s", "Reading from file stream failed");
        return_val = NULL;
        goto clean;
    }

    if (dst[stat_buf.st_size] != '\0')
        dst[stat_buf.st_size] = '\0';
    return_val = dst;

clean:
    if (fp != NULL)
    {
        if (fclose(fp) == -1)
            return_val = NULL;
    }

    if (return_val == NULL)
        str_delete(dst);

    return return_val;
}

int get_mounted_device_path(char *dst, const char *const mount_point)
{
    int return_val = 1;

    FILE *mtab = setmntent("/etc/mtab", "r");
    struct mntent *mntent_desc;

    while ((mntent_desc = getmntent(mtab)))
    {
        if (strcmp(mount_point, mntent_desc->mnt_dir) == 0)
        {
            if (!strncpy(dst, mntent_desc->mnt_fsname, strlen(mntent_desc->mnt_fsname)))
            {
                return_val = 1;
                goto end;
            }

            dst[strlen(mntent_desc->mnt_fsname)] = '\0';
            return_val = 0;
            goto end;
        }
    }

end:
    endmntent(mtab);
    return return_val;
}

int find_target_bank(char *dst
                     , const char *const mounted_device
                     , const char *const bank1_part_num
                     , const char *const bank2_part_num)
{
    int return_value = 1;

    char *mnt_device_cpy = str_copy(mounted_device);
    if (!mnt_device_cpy)
    {
        return 1;
    }

    static const char *const delim = "p";
    char *token = strtok(mnt_device_cpy, delim);
    if (token == NULL)
    {
        ERROR("%s %s %s %s", "Failed to find", delim, "in string", mnt_device_cpy);
        return_value = 1;
        goto free;
    }

    if (str_endswith(bank1_part_num, mounted_device) == 0)
    {
        snprintf(dst, strlen(token) + strlen(delim) + strlen(bank2_part_num) + 1, "%s%s%s", token, delim, bank2_part_num);
        return_value = 0;
    }
    else if (str_endswith(bank2_part_num, mounted_device) == 0)
    {
        snprintf(dst, strlen(token) + strlen(delim) + strlen(bank1_part_num) + 1, "%s%s%s", token, delim, bank1_part_num);
        return_value = 0;
    }
    else
    {
        ERROR("%s %s %s %s %s %s", "Failed to find partition number", bank1_part_num, "or", bank2_part_num, "in device file", mounted_device);
    }

free:
    str_delete(mnt_device_cpy);
    return return_value;
}

