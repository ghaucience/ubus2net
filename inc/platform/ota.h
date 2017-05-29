/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_PLATFORM_OTA_H__
#define __AYLA_PLATFORM_OTA_H__

/*
 * Setup where a new OTA image needs to be stored and open any
 * streams if needed.
 */
int platform_ota_flash_write_open(void);

/*
 * Write a chunk of the OTA image to flash. Return the number of bytes
 * successfully written.
 */
ssize_t platform_ota_flash_write(void *buf, size_t len);

/*
 * Writing of the OTA has finished. Cleanup/close any streams if
 * needed.
 */
int platform_ota_flash_close(void);

/*
 * Setup reading back of the downloaded OTA image for verification.
 */
int platform_ota_flash_read_open(void);

/*
 * Used to read back the downloaded OTA image for verification.
 * Copy the recently stored image into buf with max length of len. 'off' is the
 * offset onto the image. Return the number of bytes copied.
 */
ssize_t platform_ota_flash_read(void *buf, size_t len, size_t off);

/*
 * Apply the OTA after it's been downloaded and verified.
 */
int platform_ota_apply(void);

#endif /* __AYLA_PLATFORM_OTA_H__ */
