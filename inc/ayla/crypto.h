/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_CRYPTO_H__
#define __AYLA_CRYPTO_H__

enum crypto_rsa_key_type {
	RSA_KEY_PUBLIC,
	RSA_KEY_PRIVATE
};

/*
 * Generic cryptographic function state structure.  When initialized,
 * this structure points to functions and state for the desired algorithm.
 */
struct crypto_state {
	ssize_t (*encrypt)(void *, const void *, size_t, void *, size_t);
	ssize_t (*decrypt)(void *, const void *, size_t, void *, size_t);
	void (*context_free)(void *);
	void *context;
};


/*
 * Initialize the crypto_state for RSA encryption/decryption with the
 * specified PEM formatted key.
 */
int crypto_init_rsa(struct crypto_state *state,
	enum crypto_rsa_key_type key_type, const char *pem);

/*
 * Initialize the crypto_state for AES CBC encryption/decryption.
 * The iv parameter MUST point to a buffer with 16 bytes
 * of data to be used as the initialization vector.
 */
int crypto_init_aes(struct crypto_state *state,
	const u8 *iv, const u8 *key, size_t key_len);

/*
 * Encrypt the data in in_buf and write it to out_buf.
 * Returns the number of bytes written to out_buf, or -1 on error.
 * If out_buf is NULL, this function returns the maximum number of bytes
 * it would have written, but does nothing.
 */
ssize_t crypto_encrypt(struct crypto_state *state,
	const void *in_buf, size_t in_len,
	void *out_buf, size_t out_size);

/*
 * Decrypt the data in in_buf and write it to out_buf.
 * Returns the number of bytes written to out_buf, or -1 on error.
 * If out_buf is NULL, this function returns the maximum number of bytes
 * it would have written, but does nothing.
 */
ssize_t crypto_decrypt(struct crypto_state *state,
	const void *in_buf, size_t in_len,
	void *out_buf, size_t out_size);

/*
 * Free memory and resources associated with the crypto_state.
 */
void crypto_cleanup(struct crypto_state *state);

#endif /* __AYLA_CRYPTO_RSA_H__ */
