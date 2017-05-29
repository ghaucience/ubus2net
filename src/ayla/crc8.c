/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifdef CRC_TEST
typedef unsigned char u8;
typedef unsigned long u32;
#else
#include <ayla/utypes.h>
#endif /* CRC_TEST */

#include <stdio.h>
#include <stddef.h>
#include <ayla/crc.h>

/*
 * 4-bit CRC table.
 * Each entry in the table is the change to the CRC for each
 * of the 4 bit values.
 */
static const u8 crc8_table[16] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d
};

/*
 * Compute CRC-8 with polynomial 7 (x^8 + x^2 + x + 1).
 * MSB-first.  Use 4-bit table.
 */
u8 crc8(const void *buf, size_t len, u8 crc)
{
	const u8 *bp = buf;

	while (len-- > 0) {
		crc ^= *bp++;
		crc = (crc << 4) ^ crc8_table[crc >> 4];
		crc = (crc << 4) ^ crc8_table[crc >> 4];
	}
	return crc;
}
