/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/crypto.h>
#include <platform/crypto.h>


/*
 * Initialize a custom crypto context for RSA asymmetric encryption.
 * The key should be a string with data in the standard PEM file format.
 * Return 0 for success, and -1 for failure or if not implemented.
 */
int platform_crypto_init_rsa(struct crypto_state *state,
	enum crypto_rsa_key_type key_type, const char *pem)
{
	/* No platform-specific crypto: defaults to Openssl */
	log_debug("~~~~~~~~~~~~platform implement : [%s]", __func__);
	return -1;
}

/*
 * Initialize a custom crypto context for AES CBC symmetric encryption.
 * In-place encryption and decryption should be supported.
 * Return 0 for success, and -1 for failure or if not implemented.
 */
int platform_crypto_init_aes(struct crypto_state *state,
	const u8 *iv, const u8 *key, size_t key_len)
{
	/* No platform-specific crypto: defaults to Openssl */
	log_debug("~~~~~~~~~~~~platform implement : [%s]", __func__);
	return -1;
}
