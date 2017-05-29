/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_HEX_H__
#define __AYLA_HEX_H__

/*
 * Convert two characters of hex string to their numerical value.
 * Returns hex + the size of the byte on success or NULL on failure.
 */
const char *hex_parse_byte(const char *str, u8 *byte);

/*
 * Parse a hex encoded string of a specified length.  If delimiters are
 * present, they are detected and may be returned using the delim parameter.
 * Return -1 on failure, or the number of bytes parsed on success.
 */
ssize_t hex_parse_n(u8 *buf, size_t size, const char *str, size_t len,
	char *delim);

/*
 * Parse a hex encoded string.  If delimiters are present, they are
 * detected and may be returned using the delim parameter.
 * Return -1 on failure, or the number of bytes parsed on success.
 */
ssize_t hex_parse(u8 *buf, size_t size, const char *str, char *delim);

/*
 * Write data to a hex encoded string. If a delimiter is not needed, set to 0.
 * Returns the strlen of the string in buf on success, or -1 on failure.
 */
ssize_t hex_string(char *buf, size_t size,
	const u8 *data, size_t data_len, bool upper_case, char delim);

#endif /* __AYLA_HEX_H__ */

