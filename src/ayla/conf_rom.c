/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <ayla/utypes.h>
#include <ayla/conf_io.h>
#include <platform/conf.h>

#include <ayla/conf_rom.h>

/*
 * Attempt to load the DSN and RSA public key using the platform-specific
 * config functions.
 */
int conf_rom_load_id(void)
{
	char dsn[32];
	char key[512];
	int rc = 0;

	if (!platform_conf_read("id/dsn", dsn, sizeof(dsn))) {
		rc |= conf_set_new("id/dsn", json_string(dsn));
	}
	if (!platform_conf_read("id/rsa_pub_key", key, sizeof(key))) {
		rc |= conf_set_new("id/rsa_pub_key", json_string(key));
	}
	return rc;
}
