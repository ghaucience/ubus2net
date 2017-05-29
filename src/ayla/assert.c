/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <stdlib.h>
#include <ayla/log.h>
#include <ayla/assert.h>

void assert_failed(const char *file, int line, const char *expr)
{
	log_err("[%s:%d]  (%s)  aborting...", file, line, expr);
	abort();
}

void require_failed(const char *file, int line, const char *msg)
{
	log_err("[%s:%d]  \"%s\"  aborting...", file, line, msg);
	abort();
}
