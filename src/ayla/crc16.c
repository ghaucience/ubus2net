/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <ayla/utypes.h>

#include <stdio.h>
#include <stddef.h>
#include <ayla/crc.h>

static const u16 crc16_table[16] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};

/*
 * Compute CRC-16 with IEEE polynomial
 * LSB-first.
 */
u16 crc16(const void *buf, size_t len, u16 crc)
{
	const u8 *bp = buf;

	while (len-- > 0) {
		crc ^= *bp++ << 8;
		crc = (crc << 4) ^ crc16_table[crc >> 12];
		crc = (crc << 4) ^ crc16_table[crc >> 12];
	}
	return crc;
}
