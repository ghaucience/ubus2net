/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_UTYPES_H__
#define __AYLA_UTYPES_H__

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif /* PACKED */

#ifndef BIT
#define BIT(bit) (1U << (bit))
#endif /* BIT */

#ifndef UTYPES
#define UTYPES
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;
typedef int64_t		s64;

#define MAX_U8		UINT8_MAX
#define MAX_U16		UINT16_MAX
#define MAX_U32		UINT32_MAX
#define MAX_U64		UINT64_MAX

#define MAX_S8		INT8_MAX
#define MAX_S16		INT16_MAX
#define MAX_S32		INT32_MAX
#define MAX_S64		INT64_MAX

#define MIN_S8		INT8_MIN
#define MIN_S16		INT16_MIN
#define MIN_S32		INT32_MIN
#define MIN_S64		INT64_MIN
#endif /* UTYPES */

typedef u16		le16;
typedef u16		be16;
typedef u32		le32;
typedef u32		be32;

#ifndef __cplusplus
typedef u8 bool;
enum { false = 0, true = !0 } PACKED;
#endif /* __cplusplus */

#define OFFSET_OF(type, field) ((unsigned long)(&((type *)0)->field))
#define CONTAINER_OF(type, field, var) \
	((type *)((void *)(var) - OFFSET_OF(type, field)))

#ifndef ARRAY_LEN
#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif /* __AYLA_UTYPES_H__ */
