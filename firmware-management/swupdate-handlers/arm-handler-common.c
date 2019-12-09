/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "arm-handler-common.h"
#include "swupdate/swupdate.h"
#include "swupdate/util.h"

void *malloc_or_abort(const size_t size)
{
    char *dst = malloc(sizeof dst * size);
    if (!dst)
    {
        ERROR("%s", "Failed to allocate memory");
        abort();
    }

    return dst;
}

char *str_copy_to_new(const char *const src)
{
    const size_t dst_size = strlen(src) + 1;
    char *dst = malloc_or_abort(dst_size);
    return strcpy(dst, src);
}

int str_endswith(const char *const substr, const char *const fullstr)
{
    const size_t substr_len = strlen(substr);
    const size_t fullstr_len = strlen(fullstr);
    if (substr_len > fullstr_len) return 1;
    const size_t endlen = fullstr_len - substr_len;
    return strcmp(&fullstr[endlen], substr);
}

char *read_file_to_new_str(const char *const filepath)
{
    struct stat stat_buf;
    if (stat(filepath, &stat_buf) == -1)
    {
        ERROR("%s %s: %s", "Failed to stat file", filepath, strerror(errno));
        return NULL;
    }

    if (stat_buf.st_size < 0)
    {
        ERROR("%s %s", filepath, "has a negative size");
        return NULL;
    }

    char *dst = malloc_or_abort((size_t)stat_buf.st_size + 1);
    char *return_val;
    FILE *const fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        ERROR("%s %s: %s", "Failed to open file", filepath, strerror(errno));
        return_val = NULL;
        goto clean;
    }

    fread(dst, 1, (size_t)stat_buf.st_size, fp);
    if (feof(fp) != 0 || ferror(fp) != 0)
    {
        ERROR("%s %s", "Failed to read from file", filepath);
        return_val = NULL;
        goto clean;
    }

    dst[stat_buf.st_size] = '\0';
    return_val = dst;

clean:
    if (fp != NULL)
    {
        if (fclose(fp) == -1)
        {
            ERROR("%s %s", "Failed to close file descriptor for file", filepath);
            return_val = NULL;
        }
    }

    if (return_val == NULL)
        free(dst);

    return return_val;
}

int get_mounted_device(char *const dst, const char *const mount_point, const size_t dst_size)
{
    int return_val = -1;

    FILE *mtab = setmntent("/etc/mtab", "r");
    if (!mtab)
    {
        ERROR("%s", "Failed to open mtab");
        return -1;
    }

    struct mntent *mntent_desc;

    while ((mntent_desc = getmntent(mtab)))
    {
        if (strcmp(mount_point, mntent_desc->mnt_dir) == 0)
        {
            strncpy(dst, mntent_desc->mnt_fsname, dst_size);

            if (dst[dst_size] != '\0')
            {
                ERROR("%s %s", mntent_desc->mnt_fsname, "could not fit into destination buffer and was truncated");
                return_val = -1;
                goto end;
            }

            return_val = 0;
            goto end;
        }
    }

end:
    endmntent(mtab);
    return return_val;
}

int find_target_partition(char *const dst
        , const char *const mounted_partition
        , const char *const bank1_part_num
        , const char *const bank2_part_num
        , const size_t dst_size)
{
    int return_value = -1;

    char *mnt_device_cpy = str_copy_to_new(mounted_partition);

    static const char *const delim = "p";
    char *token = strtok(mnt_device_cpy, delim);
    if (token == NULL)
    {
        ERROR("%s %s %s", "Failed to find", delim, "in string");
        return_value = -1;
        goto free;
    }

    if (str_endswith(bank1_part_num, mounted_partition) == 0)
    {
        const int num_written = snprintf(dst, dst_size, "%s%s%s", token, delim, bank2_part_num);
        if (num_written < 0)
        {
            ERROR("%s", "There was an output error when writing to the destination buffer");
            return_value = -1;
            goto free;
        }

        if ((size_t)num_written >= dst_size)
        {
            ERROR("%s", "bootflags filepath is larger than the destination buffer size");
            return_value = -1;
            goto free;
        }

        return_value = 0;
    }
    else if (str_endswith(bank2_part_num, mounted_partition) == 0)
    {
        const int num_written = snprintf(dst, dst_size, "%s%s%s", token, delim, bank1_part_num);
        if (num_written < 0)
        {
            ERROR("%s", "There was an output error when writing to the destination buffer");
            return_value = -1;
            goto free;
        }

        if ((size_t)num_written >= dst_size)
        {
            ERROR("%s", "Target partition filepath is larger than the destination buffer size");
            return_value = -1;
            goto free;
        }

        return_value = 0;
    }
    else
    {
        ERROR("%s %s %s %s %s", "Failed to find partition number", bank1_part_num, "or", bank2_part_num, "in device file");
    }

free:
    free(mnt_device_cpy);
    return return_value;
}

int get_part_info_filepath(char *const dst, const char *const file_name, const size_t dst_size)
{
    const int num_written = snprintf(
        dst
        , dst_size
        , "%s/%s"
        , PART_INFO_DIR
        , file_name);

    if (num_written < 0)
    {
        ERROR("%s", "There was an output error when writing to the destination buffer");
        return -1;
    }

    if ((size_t)num_written >= dst_size)
    {
        ERROR("%s", "Part info filepath is larger than the destination buffer size");
        return -1;
    }

    return 0;
}

char *read_part_info_file_to_new_str(const char *const file_name)
{
    char part_info_filepath[PATH_MAX];
    if (get_part_info_filepath(part_info_filepath, file_name, PATH_MAX) == -1)
        return NULL;
    return read_file_to_new_str(part_info_filepath);
}

int get_bootflag_file_path(char *const bootflags_file_path, const char *const filename, const size_t size)
{
    int num_written = snprintf(bootflags_file_path, size, "%s/%s", BOOTFLAGS_DIR, filename);
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

int write_bootflag_file(const char *const filename)
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
    if (get_bootflag_file_path(bootflags_file_path, filename, PATH_MAX) == -1)
    {
        return -1;
    }

    const int fd = open(bootflags_file_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1)
    {
        ERROR("%s %s: %s", "Failed to open file", bootflags_file_path, strerror(errno));
        return -1;
    }

    if (close(fd) == -1)
    {
        WARN("%s %s: %s", "Error closing file descriptor for file", bootflags_file_path, strerror(errno));
    }

    sync();

    return 0;
}

int remove_bootflag_file(const char *const filename)
{
    char bootflags_file_path[PATH_MAX];
    if (get_bootflag_file_path(bootflags_file_path, filename, PATH_MAX) < 0)
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

int copy_image_and_sync(struct img_type *img, const char *const device_filepath)
{
    int ret_val = 0;
    int fd = openfileoutput(device_filepath);
    if (fd < 0)
    {
        ERROR("%s %s", "Failed to open target device file", device_filepath);
        return -1;
    }

    if (copyimage(&fd, img, NULL) < 0)
    {
        ERROR("%s %s %s %s", "Failed to copy", img->fname, "to target device", device_filepath);
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
