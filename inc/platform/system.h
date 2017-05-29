/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_PLATFORM_SYSTEM_H__
#define __AYLA_PLATFORM_SYSTEM_H__

#include <netinet/ether.h>

#define PLATFORM_HW_ID_MAX_SIZE	(64) /* Enough space any possible HW IDs */

/*
 * Get the mac address of the device
 */
int platform_get_mac_addr(struct ether_addr *addr);

/*
 * Get a unique hardware id of the device
 */
int platform_get_hw_id(char *buf, size_t size);

/*
 * Configure cloud LED if available on the platform
 */
void platform_configure_led(bool cloud_up, bool registered,
	bool reg_window_open);

/*
 * Configures the system when setup mode is enabled or disabled.
 */
void platform_apply_setup_mode(bool enable);

/*
 * Perform any actions needed to factory reset the system.  This is invoked
 * after devd has performed all operations needed to factory reset Ayla modules.
 * Actions that could be performed here might include setting LEDs to indicate
 * the reset status or running an external script.  During a normal factory
 * reset, platform_reset() will be called after this function returns.
 */
void platform_factory_reset(void);

/*
 * Reboot the system.
 */
void platform_reset(void);


#endif /* __AYLA_PLATFORM_SYSTEM_H__ */
