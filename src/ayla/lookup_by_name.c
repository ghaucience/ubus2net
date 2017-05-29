/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <string.h>
#include <ayla/nameval.h>

int lookup_by_name(const struct name_val *table, const char *name)
{
	for (; table->name != NULL; table++) {
		if (!strcasecmp(table->name, name)) {
			return table->val;
		}
	}
	return -1;
}
