/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <ayla/log.h>

#ifdef _WIN32
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

/*
 * Given a file path, returns the directory of the file. If dest buf is given
 * the directory is copied into it. If dest is the same as path,
 * this path/dest is converted in-place to the directory path.
 */
char *file_get_dir(const char *path, char *dest, size_t size)
{
	char *slash;

	if (dest != path) {
		snprintf(dest, size, "%s", path);
	}
	slash = strrchr(dest, FILE_SEPARATOR);
	if (!slash) {
		/* return current dir */
		snprintf(dest, size, ".");
	} else if (dest == slash) {
		snprintf(dest, size, "%c", FILE_SEPARATOR);
	} else {
		*slash = '\0';
	}
	return dest;
}

/*
 * Given a file path, returns the name of the file;
 */
const char *file_get_name(const char *path)
{
	char *slash;

	slash = strrchr(path, '/');
	if (!slash) {
		if (!strcmp(path, ".")) {
			/* not a valid file */
			return NULL;
		}
		/* file in current dir */
		return path;
	}
	return slash + 1;
}

/*
 * Cleanup a dir path. If it ends in a '/', remove it.
 */
char *file_clean_path(char *path)
{
	int len;

	if (!path) {
		return NULL;
	}
	len = strlen(path);
	if (len <= 1) {
		/* for cases where path is the current dir */
		return path;
	}
	if (path[len - 1] == '/') {
		path[len - 1] = '\0';
	}

	return path;
}

/*
 * Return this size of a file
 */
ssize_t file_get_size(const char *path)
{
	int fd;
	size_t file_size;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		log_err("open failed for %s: %m", path);
		return -1;
	}
	file_size = lseek(fd, 0, SEEK_END);
	close(fd);

	return file_size;
}

/*
 * Copy the contents of src into dest. Returns -1 on err or the number of bytes
 * copied on success.
 */
ssize_t file_copy(const char *src, const char *dest)
{
	struct stat stat_buf;
	int input_fd;
	int output_fd;
	int rc;

	input_fd = open(src, O_RDONLY);
	if (input_fd == -1) {
		log_err("open failed for %s: %m", src);
		return -1;
	}
	fstat(input_fd, &stat_buf);
	output_fd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, stat_buf.st_mode);
	if (output_fd == -1) {
		log_err("open failed for %s: %m", dest);
		close(input_fd);
		return -1;
	}
	rc = sendfile(output_fd, input_fd, NULL, stat_buf.st_size);
	if (rc == -1) {
		log_err("sendfile err %m for copying %s to %s", src, dest);
	}
	close(input_fd);
	close(output_fd);

	return rc;
}

/*
 * Generates a directory including parent dirs.
 * Returns 0 on success and -1 on failure.  Errno is set if return is -1.
 */
int file_create_dir(const char *path, int mode)
{
	char buf[PATH_MAX];
	char *delim;

	snprintf(buf, sizeof(buf), "%s", path);
	delim = strchr(buf, FILE_SEPARATOR);

	/* walk path and create parent directories */
	while (delim) {
		/* skip leading slash */
		if (delim == buf) {
			delim = strchr(delim + 1, FILE_SEPARATOR);
			continue;
		}
		*delim = '\0';
		if (access(buf, F_OK) < 0 && mkdir(buf, mode) < 0) {
			return -1;
		}
		*delim = FILE_SEPARATOR;
		delim = strchr(delim + 1, FILE_SEPARATOR);
	}

	if (access(path, F_OK) < 0) {
		if (mkdir(path, mode) < 0) {
			return -1;
		}
	}
	return 0;
}

/*
 * Open a file, or create it, if it does not exist.
 * Returns 0 on success, or -1 on failure.
 */
int file_touch(const char *path)
{
	FILE *fp;

	fp = fopen(path, "w");
	if (fp == NULL) {
		log_err("file open err %s: %m", path);
		return -1;
	}
	fclose(fp);
	return 0;
}

/*
 * Returns non-zero if given path is a directory.
 */
int file_is_dir(const char *path)
{
	DIR *dir;

	dir = opendir(path);
	if (!dir) {
		return 0;
	}
	closedir(dir);
	return 1;
}
