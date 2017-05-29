/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_BASE64_H__
#define __AYLA_BASE64_H__

/*
 * Base64 encode
 */
char *base64_encode(const char *input, size_t length, size_t *outlen);

/*
 * Base64 decode
 */
char *base64_decode(const char *input, size_t length, size_t *outlen);

#endif /* __AYLA_BASE64_H__ */

