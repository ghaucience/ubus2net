/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#ifndef __AYLA_CONF_ROM_H__
#define __AYLA_CONF_ROM_H__

#include <jansson.h>

/*
 * Attempt to load the DSN and RSA public key using the platform-specific
 * config functions.
 */
int conf_rom_load_id(void);

#endif /* __AYLA_CONF_ROM_H__ */
