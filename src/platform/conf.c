/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <platform/conf.h>


/*
 * Read a platform-specific configuration item.  This could be used
 * to read system information such as DSN and RSA key from ROM, or for
 * some application-specific purpose.  The item is identified with a
 * slash-delimited path in the same format as internal config file paths used
 * with the conf_io library.
 * Return 0 for success, and -1 for failure or if not implemented.
 */
int platform_conf_read(const char *path, char *buf, size_t buf_size)
{
	log_debug("~~~~~~~~~~~~platform implement : [%s]", __func__);
	return -1;
}

/*
 * Write a platform-specific configuration item.  This could be used
 * to flash system information such as DSN and RSA key for device provisioning,
 * or for some application-specific purpose.
 * Return 0 for success, and -1 for failure or if not implemented.
 */
int platform_conf_write(const char *path, const char *value)
{
	log_debug("~~~~~~~~~~~~platform implement : [%s]", __func__);
	return -1;
}
