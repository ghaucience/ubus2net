/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_CRC_H__
#define __AYLA_CRC_H__

#define CRC8_POLY	7
#define CRC8_INIT	0xffU

u8 crc8(const void *buf, size_t len, u8 init_crc);

#define	CRC16_POLY	0x1021U		/* CCITT polynomial */
#define	CRC16_INIT	0xffffU

u16 crc16(const void *buf, size_t len, u16 init_crc);

#define	CRC32_POLY	0xedb88320U	/* polynomial for sending LSB first */
#define	CRC32_INIT	0xffffffffU

u32 crc32(const void *buf, size_t len, u32 init_crc);

#endif /* __AYLA_CRC_H__ */

