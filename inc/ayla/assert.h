/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_ASSERT_H__
#define __AYLA_ASSERT_H__

#ifndef NDEBUG
void assert_failed(const char *file, int line, const char *expr);

#define ASSERT(expr)					\
	do {						\
		if (!(expr)) {				\
			assert_failed(__FILE__, __LINE__, #expr);	\
		}					\
	} while (0)

#define ASSERT_NOTREACHED()				\
	assert_failed(__FILE__, __LINE__, "not reached");

#else	/* NDEBUG disables runtime assertions */
#define ASSERT(expr)
#define ASSERT_NOTREACHED()
#endif

/*
 * Force a compile error if an expression is false or can't be evalutated.
 */
#define ASSERT_COMPILE(name, expr) \
	extern char __ASSERT_##__name[(expr) ? 1 : -1]

/*
 * Force a compile error if size of type is not as expected.
 */
#define ASSERT_SIZE(kind, name, size) \
	ASSERT_COMPILE(kind ## name, sizeof(kind name) == (size))


void require_failed(const char *file, int line, const char *msg);

#define REQUIRE(expr, msg)					\
	do {							\
		if (!(expr)) {					\
			require_failed(__FILE__, __LINE__, msg);\
		}						\
	} while (0)

#define REQUIRE_FAILED(msg)					\
	require_failed(__FILE__, __LINE__, msg);

/*
 * Common REQUIRE messages
 */
#define REQUIRE_MSG_ALLOCATION	"memory allocation failed"
#define REQUIRE_MSG_NULL	"NULL pointer"
#define REQUIRE_MSG_BUF_SIZE	"buffer overflow"

#endif /* __AYLA_ASSERT_H__ */
