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

const char *lookup_by_val(const struct name_val *table, int val)
{
	for (; table->name != NULL; table++) {
		if (table->val == val) {
			return table->name;
		}
	}
	return NULL;
}
