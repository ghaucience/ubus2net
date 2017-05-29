/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_JSON_PARSER_H__
#define __AYLA_JSON_PARSER_H__

#include <jansson.h>

int json_get_bool(const json_t *obj, const char *name, bool *value);

int json_get_int(const json_t *obj, const char *name, int *value);
int json_get_int64(const json_t *obj, const char *name, s64 *value);

int json_get_uint(const json_t *obj, const char *name, unsigned *value);
int json_get_uint8(const json_t *obj, const char *name, u8 *value);
int json_get_uint16(const json_t *obj, const char *name, u16 *value);
int json_get_uint64(const json_t *obj, const char *name, u64 *value);

int json_get_double(const json_t *obj, const char *name, double *value);

const char *json_get_string(const json_t *obj, const char *name);
char *json_get_string_dup(const json_t *obj, const char *name);
ssize_t json_get_string_copy(const json_t *obj, const char *name,
	char *buf, size_t size);

#endif /* __AYLA_JSON_PARSER_H__ */
