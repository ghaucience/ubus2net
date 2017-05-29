/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_NAMEVAL_H__
#define __AYLA_NAMEVAL_H__

struct name_val {
	const char *name;
	int val;
};

int lookup_by_name(const struct name_val *, const char *);
const char *lookup_by_val(const struct name_val *, int);

#endif /* __AYLA_NAMEVAL_H__ */
