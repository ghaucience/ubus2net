/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdio.h>
#include <string.h>
#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/json_parser.h>

static int json_get_integer(const json_t *obj, const char *name,
	json_int_t *value)
{
	const json_t *elem;

	if (!json_is_object(obj)) {
		return -1;
	}
	elem = json_object_get(obj, name);
	if (!json_is_integer(elem)) {
		return -1;
	}
	*value = json_integer_value(elem);
	return 0;
}

int json_get_bool(const json_t *obj, const char *name, bool *value)
{
	const json_t *elem;
	json_int_t int_val;

	if (!json_is_object(obj)) {
		return -1;
	}
	elem = json_object_get(obj, name);
	/* Handle boolean type JSON element */
	if (json_is_boolean(elem)) {
		*value = json_boolean_value(elem) ? true : false;
		return 0;
	}
	/* Handle integer type JSON element with value 1 or 0 */
	if (json_is_integer(elem)) {
		int_val = json_integer_value(elem);
		if (int_val == 0) {
			*value = false;
			return 0;
		}
		if (int_val == 1) {
			*value = true;
			return 0;
		}
	}
	return -1;
}

int json_get_int(const json_t *obj, const char *name, int *value)
{
	json_int_t val;

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	if (val < MIN_S32 || val > MAX_S32) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_int64(const json_t *obj, const char *name, s64 *value)
{
	json_int_t val;

	ASSERT(sizeof(json_int_t) >= sizeof(*value));

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_uint(const json_t *obj, const char *name, unsigned *value)
{
	json_int_t val;

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	if (val < 0 || val > MAX_U32) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_uint8(const json_t *obj, const char *name, u8 *value)
{
	json_int_t val;

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	if (val < 0 || val > MAX_U8) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_uint16(const json_t *obj, const char *name, u16 *value)
{
	json_int_t val;

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	if (val < 0 || val > MAX_U16) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_uint64(const json_t *obj, const char *name, u64 *value)
{
	json_int_t val;

	ASSERT(sizeof(json_int_t) >= sizeof(*value));

	if (json_get_integer(obj, name, &val) < 0) {
		return -1;
	}
	if (val < 0) {
		return -1;
	}
	*value = val;
	return 0;
}

int json_get_double(const json_t *obj, const char *name, double *value)
{
	const json_t *elem;

	if (!json_is_object(obj)) {
		return -1;
	}
	elem = json_object_get(obj, name);
	if (!json_is_number(elem)) {
		return -1;
	}
	*value = json_number_value(elem);
	return 0;
}

/*
 * Return pointer to string in the json parse.
 * Note that this can be used only until json_decref() is called on the root.
 */
const char *json_get_string(const json_t *obj, const char *name)
{
	const json_t *elem;

	if (!json_is_object(obj)) {
		return NULL;
	}
	elem = json_object_get(obj, name);
	if (!json_is_string(elem)) {
		return NULL;
	}
	return json_string_value(elem);
}

char *json_get_string_dup(const json_t *obj, const char *name)
{
	const char *str;

	str = json_get_string(obj, name);
	if (!str) {
		return NULL;
	}
	return strdup(str);
}

ssize_t json_get_string_copy(const json_t *obj, const char *name,
	char *buf, size_t size)
{
	const char *str;
	ssize_t rc;

	str = json_get_string(obj, name);
	if (!str) {
		return -1;
	}
	rc = snprintf(buf, size, "%s", str);
	if (rc >= size) {
		return -1;
	}
	return rc;
}
