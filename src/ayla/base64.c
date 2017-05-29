/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <string.h>

#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>	/* for base64 decode */

/*
 * Base64 decode
 */
char *base64_decode(const char *input, size_t length, size_t *outlen)
{
	BIO *b64;
	BIO *bmem;
	void *buffer;
	int rc;

	buffer = malloc(length);
	memset(buffer, 0, length);

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new_mem_buf((void *)input, -1);
	bmem = BIO_push(b64, bmem);

	rc = BIO_read(bmem, buffer, length);

	BIO_free_all(bmem);
	if (rc <= 0) {
		free(buffer);
		buffer = NULL;
		rc = 0;
	}
	if (outlen) {
		*outlen = rc;
	}
	return buffer;
}

/*
 * Base64 encode
 */
char *base64_encode(const char *input, size_t length, size_t *outlen)
{
	BIO *bmem;
	BIO *b64;
	BUF_MEM *bptr;
	char *buffer;

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new(BIO_s_mem());

	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	(void)BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	if (outlen) {
		*outlen = bptr->length;
	}
	if (!bptr->length) {
		return NULL;
	}
	buffer = malloc(bptr->length + 1);
	if (!buffer) {
		return NULL;
	}
	memcpy(buffer, bptr->data, bptr->length);
	buffer[bptr->length] = 0;

	BIO_free_all(b64);

	return buffer;
}

