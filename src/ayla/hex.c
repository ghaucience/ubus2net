/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdlib.h>
#include <string.h>

#include <ayla/utypes.h>
#include <ayla/hex.h>


static char hex_nibble_to_ascii(u8 nibble, bool upper_case)
{
	nibble &= 0xF;
	if (nibble < 10) {
		return nibble + '0';
	}
	return nibble - 10 + (upper_case ? 'A' : 'a');
}

/*
 * Convert a hex character to its numerical value
 */
static s8 hex_ascii_to_nibble(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return -1;
}

/*
 * Convert two characters of hex string to their numerical value.
 * Returns hex + the size of the byte on success or NULL on failure.
 */
const char *hex_parse_byte(const char *str, u8 *byte)
{
	s8 high, low;

	high = hex_ascii_to_nibble(*str);
	if (high < 0) {
		return NULL;
	}
	++str;
	low = hex_ascii_to_nibble(*str);
	if (low < 0) {
		return NULL;
	}
	*byte = (high << 4) | low;
	return ++str;
}

/*
 * Parse a hex encoded string of a specified length.  If delimiters are
 * present, they are detected and may be returned using the delim parameter.
 * Return -1 on failure, or the number of bytes parsed on success.
 */
ssize_t hex_parse_n(u8 *buf, size_t size, const char *str, size_t len,
	char *delim)
{
	size_t nbytes = 0;
	char found_delim = 0;

	if (len < 2) {
		return -1;
	}
	/* Detect delimiter after first byte */
	if (len > 4) {
		if (hex_ascii_to_nibble(str[2]) < 0) {
			found_delim = str[2];
		}
	}
	/* Check length and buffer size (str must have complete hex bytes) */
	if (found_delim) {
		if ((len - 2) % 3 || size < (len - 2) / 3 + 1) {
			return -1;
		}
	} else {
		if (len % 2 || size < len / 2) {
			return -1;
		}
	}
	for (;;) {
		if (nbytes >= size) {
			return -1;
		}
		str = hex_parse_byte(str, buf + nbytes);
		if (!str) {
			return -1;
		}
		++nbytes;
		len -= 2;
		if (!len) {
			/* Done */
			break;
		}
		/* Check delimiter */
		if (found_delim) {
			if (*str != found_delim) {
				return -1;
			}
			++str;
			--len;
		}
	}
	if (delim) {
		*delim = found_delim;
	}
	return nbytes;
}

/*
 * Parse a hex encoded string.  If delimiters are present, they are
 * detected and may be returned using the delim parameter.
 * Return -1 on failure, or the number of bytes parsed on success.
 */
ssize_t hex_parse(u8 *buf, size_t size, const char *str, char *delim)
{
	return hex_parse_n(buf, size, str, strlen(str), delim);
}

/*
 * Write data to a hex encoded string. If a delimiter is not needed, set to 0.
 * Returns the strlen of the string in buf on success, or -1 on failure.
 */
ssize_t hex_string(char *buf, size_t size,
	const u8 *data, size_t data_len, bool upper_case, char delim)
{
	size_t len;

	len = data_len * 2;
	if (delim) {
		len += data_len - 1;
	}
	if (size <= len) {
		return -1;
	}
	while (data_len--) {
		*buf++ = hex_nibble_to_ascii(*data >> 4, upper_case);
		*buf++ = hex_nibble_to_ascii(*data, upper_case);
		++data;
		if (delim && data_len) {
			*buf++ = delim;
		}
	}
	*buf = '\0';
	return len;
}
