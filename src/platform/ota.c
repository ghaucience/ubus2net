/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <platform/ota.h>

#define PLATFORM_OTA_FILE_PATH	"/tmp/fw.bin"

static int ota_fd = -1;


/*
 * Setup where a new OTA image needs to be stored and open any
 * streams if needed.
 */
int platform_ota_flash_write_open(void)
{
	/*
	 * Sample implementation that writes the OTA data to a file.
	 */
	ota_fd = open(PLATFORM_OTA_FILE_PATH, O_WRONLY | O_CREAT,
	    S_IRUSR | S_IWUSR);
	if (ota_fd < 0) {
		log_err("file cannot be opened for writing - %m");
		return -1;
	}
	log_debug("opened file %s for writing", PLATFORM_OTA_FILE_PATH);
	return 0;
}

/*
 * Write a chunk of the OTA image to flash. Return the number of bytes
 * successfully written.
 */
ssize_t platform_ota_flash_write(void *buf, size_t len)
{
	/*
	 * Sample implementation that writes the OTA data to a file.
	 */
	ssize_t ret;

	if (ota_fd < 0) {
		log_err("invalid fd.");
		return -1;
	}
	ret = write(ota_fd, buf, len);
	if (ret < 0) {
		log_err("file cannot be written to - %m");
		return -1;
	}
	return ret;
}

/*
 * Writing of the OTA has finished. Cleanup/close any streams if
 * needed.
 */
int platform_ota_flash_close(void)
{
	/*
	 * Sample implementation that writes the OTA data to a file.
	 */
	int ret = close(ota_fd);

	if (ret < 0) {
		log_err("file cannot be closed - %m");
	}
	return ret;
}

/*
 * Setup reading back of the downloaded OTA image for verification.
 */
int platform_ota_flash_read_open(void)
{
	/*
	 * Sample implementation that writes the OTA data to a file.
	 */
	ota_fd = open(PLATFORM_OTA_FILE_PATH, O_RDONLY,
	    S_IRUSR | S_IWUSR);
	if (ota_fd < 0) {
		log_err("file cannot be opened for reading - %m");
		return -1;
	}
	return 0;
}

/*
 * Used to read back the downloaded OTA image for verification.
 * Copy the recently stored image into buf with max length of len. 'off' is the
 * offset onto the image. Return the number of bytes copied.
 */
ssize_t platform_ota_flash_read(void *buf, size_t len, size_t off)
{
	/*
	 * Sample implementation that writes the OTA data to a file.
	 */
	ssize_t ret;

	if (ota_fd < 0) {
		log_err("invalid fd.");
		return -1;
	}
	ret = read(ota_fd, buf, len);
	if (ret < 0) {
		log_err("file cannot be read - %m");
		return -1;
	}

	return ret;
}

/*
 * Apply the OTA after it's been downloaded and verified.
 */
int platform_ota_apply(void)
{
	log_debug("OAT -------> [%s]...", __func__);

	char buf[256];
	sprintf(buf, "mtd write %s firmware; reboot -f;", __func__);
	system(buf);

	log_debug("OAT -------> [%s] complate...", __func__);
	return 0;
}
