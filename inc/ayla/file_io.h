/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_FILE_IO_H__
#define __AYLA_FILE_IO_H__

/*
 * Given a file path, returns the directory of the file. If dest buf is given
 * the directory is copied into it. If dest is the same as path,
 * this path/dest is converted in-place to the directory path.
 */
char *file_get_dir(const char *path, char *dest, size_t size);

/*
 * Given a file path, returns the name of the file;
 */
const char *file_get_name(const char *path);

/*
 * Cleanup a dir path. If it ends in a '/', remove it.
 */
char *file_clean_path(char *path);

/*
 * Copy the contents of src into dest. Returns -1 on err or the number of bytes
 * copied on success.
 */
ssize_t file_copy(const char *src, const char *dest);

/*
 * Generates a directory including parent dirs.
 * Returns 0 on success and -1 on failure.  Errno is set if return is -1.
 */
int file_create_dir(const char *path, int mode);

/*
 * Open a file, or create it, if it does not exist.
 * Returns 0 on success, or -1 on failure.
 */
int file_touch(const char *path);

/*
 * Return this size of a file
 */
ssize_t file_get_size(const char *path);

/*
 * Return 1 if path is a directory, otherwise 0.
 */
int file_is_dir(const char *path);

#endif /* __AYLA_FILE_IO_H__ */
