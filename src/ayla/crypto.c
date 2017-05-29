/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <stdio.h>
#include <string.h>

#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/assert.h>
#include <ayla/crypto.h>
#include <platform/crypto.h>


#define CRYPTO_OPENSSL_LOG_ERR(msg, err)	\
	log_err(msg ": %s() - %s",		\
	    ERR_func_error_string(err),	\
	    ERR_reason_error_string(err))

/*
 * Openssl RSA crypto context.
 */
struct crypto_ctx_rsa_openssl {
	RSA *rsa;
	enum crypto_rsa_key_type key_type;
};

/*
 * Openssl AES crypto context.
 */
struct crypto_ctx_aes_openssl {
	AES_KEY encrypt_key;
	AES_KEY decrypt_key;
	u8 iv[AES_BLOCK_SIZE];
};

/*
 * Load Openssl common resources and config once per process.
 */
static void crypto_init_openssl(void)
{
	static bool loaded;

	if (!loaded) {
		ERR_load_crypto_strings();
		OPENSSL_config(NULL);
		loaded = true;
	}
}

/*
 * Default RSA encrypt function.  Uses PKCS #1 padding.
 */
static ssize_t crypto_encrypt_rsa_openssl(void *context,
	const void *in_buf, size_t in_size,
	void *out_buf, size_t out_size)
{
	struct crypto_ctx_rsa_openssl *ctx =
	    (struct crypto_ctx_rsa_openssl *)context;
	unsigned long err;
	size_t out_len;
	int rc = -1;

	out_len = RSA_size(ctx->rsa);
	if (!out_buf) {
		return out_len;
	}
	if (out_len > out_size) {
		log_err("out_buf must be at least %zu bytes", out_len);
		return -1;
	}
	switch (ctx->key_type) {
	case RSA_KEY_PUBLIC:
		rc = RSA_public_encrypt((int)in_size, (unsigned char *)in_buf,
		    (unsigned char *)out_buf, ctx->rsa, RSA_PKCS1_PADDING);
		break;
	case RSA_KEY_PRIVATE:
		rc = RSA_private_encrypt((int)in_size, (unsigned char *)in_buf,
		    (unsigned char *)out_buf, ctx->rsa, RSA_PKCS1_PADDING);
		break;
	}
	if (rc < 0) {
		err = ERR_peek_last_error();
		CRYPTO_OPENSSL_LOG_ERR("encrypt failed", err);
		return -1;
	}
	return rc;
}

/*
 * Default RSA decrypt function.  Uses PKCS #1 padding.
 */
static ssize_t crypto_decrypt_rsa_openssl(void *context,
	const void *in_buf, size_t in_size,
	void *out_buf, size_t out_size)
{
	struct crypto_ctx_rsa_openssl *ctx =
	    (struct crypto_ctx_rsa_openssl *)context;
	unsigned long err;
	size_t out_len;
	int rc = -1;

	/* Max decrypt output size according to Openssl docs */
	out_len = in_size - 12;
	if (!out_buf) {
		return out_len;
	}
	if (out_len > out_size) {
		log_err("out_buf must be at least %zu bytes", out_len);
		return -1;
	}
	switch (ctx->key_type) {
	case RSA_KEY_PUBLIC:
		rc = RSA_public_decrypt((int)in_size, (unsigned char *)in_buf,
		    (unsigned char *)out_buf, ctx->rsa, RSA_PKCS1_PADDING);
		break;
	case RSA_KEY_PRIVATE:
		rc = RSA_private_decrypt((int)in_size, (unsigned char *)in_buf,
		    (unsigned char *)out_buf, ctx->rsa, RSA_PKCS1_PADDING);
		break;
	}
	if (rc < 0) {
		err = ERR_peek_last_error();
		CRYPTO_OPENSSL_LOG_ERR("decrypt failed", err);
		return -1;
	}
	return rc;
}

/*
 * Default RSA cleanup function.
 */
static void crypto_cleanup_rsa_openssl(void *context)
{
	struct crypto_ctx_rsa_openssl *ctx =
	    (struct crypto_ctx_rsa_openssl *)context;

	if (!ctx) {
		return;
	}
	if (ctx->rsa) {
		RSA_free(ctx->rsa);
	}
	free(ctx);
}

/*
 * Default RSA init function: Openssl.
 */
static int crypto_init_rsa_openssl(struct crypto_state *state,
	enum crypto_rsa_key_type key_type, const char *pem)
{
	unsigned long err;
	BIO *bio = NULL;
	struct crypto_ctx_rsa_openssl *ctx;
	int rc = -1;

	/* Load Openssl common modules */
	crypto_init_openssl();

	if (!pem) {
		log_err("missing key");
		return -1;
	}
	ctx = (struct crypto_ctx_rsa_openssl *)malloc(sizeof(*ctx));
	if (!ctx) {
		log_err("malloc failed");
		return -1;
	}
	ctx->key_type = key_type;
	bio = BIO_new_mem_buf((void *)pem, -1);
	if (!bio) {
		log_err("failed to load PEM key");
		goto error;
	}
	switch (key_type) {
	case RSA_KEY_PUBLIC:
		/*
		 * Read key in AFS-preferred format first.  If that fails, use
		 * the more standard RSA format.
		 */
		ctx->rsa = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);
		if (!ctx->rsa) {
			/* Report first error if both fail */
			err = ERR_peek_last_error();
			BIO_reset(bio);
			ctx->rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL,
			    NULL);
			if (!ctx->rsa) {
				CRYPTO_OPENSSL_LOG_ERR(
				    "public key import failed", err);
				goto error;
			}
		}
		break;
	case RSA_KEY_PRIVATE:
		ctx->rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
		if (!ctx->rsa) {
			err = ERR_peek_last_error();
			CRYPTO_OPENSSL_LOG_ERR("private key import failed",
			    err);
			goto error;
		}
		break;
	}
	if (!ctx->rsa->n) {
		log_err("RSA key init failed");
		goto error;
	}
	state->encrypt = crypto_encrypt_rsa_openssl;
	state->decrypt = crypto_decrypt_rsa_openssl;
	state->context_free = crypto_cleanup_rsa_openssl;
	state->context = (void *)ctx;
	rc = 0;
error:
	if (bio) {
		BIO_vfree(bio);
	}
	if (rc < 0) {
		crypto_cleanup_rsa_openssl(ctx);
	}
	return rc;
}

/*
 * Internal AES encrypt/decrypt implementation.  AES_cbc_encrypt() supports
 * in-place encryption and decryption.
 */
static ssize_t crypto_aes_openssl(void *context, int type,
	const void *in_buf, size_t in_size,
	void *out_buf, size_t out_size)
{
	struct crypto_ctx_aes_openssl *ctx =
	    (struct crypto_ctx_aes_openssl *)context;
	size_t out_len;

	/* Input size must be a multiple of AES block size */
	out_len = in_size & ~(AES_BLOCK_SIZE - 1);
	if (!out_buf) {
		return out_len;
	}
	if (out_len > out_size) {
		log_err("out_buf must be at least %zu bytes", out_len);
		return -1;
	}
	switch (type) {
	case AES_ENCRYPT:
		AES_cbc_encrypt((unsigned char *)in_buf,
		    (unsigned char *)out_buf, out_len,
		    &ctx->encrypt_key, ctx->iv, AES_ENCRYPT);
		break;
	case AES_DECRYPT:
		AES_cbc_encrypt((unsigned char *)in_buf,
		    (unsigned char *)out_buf, out_len,
		    &ctx->decrypt_key, ctx->iv, AES_DECRYPT);
		break;
	default:
		ASSERT_NOTREACHED();
	}
	return out_len;
}

/*
 * Default AES encrypt function.
 */
static inline ssize_t crypto_encrypt_aes_openssl(void *context,
	const void *in_buf, size_t in_size,
	void *out_buf, size_t out_size)
{
	return crypto_aes_openssl(context, AES_ENCRYPT, in_buf, in_size,
	    out_buf, out_size);
}

/*
 * Default AES decrypt function.
 */
static inline ssize_t crypto_decrypt_aes_openssl(void *context,
	const void *in_buf, size_t in_size,
	void *out_buf, size_t out_size)
{
	return crypto_aes_openssl(context, AES_DECRYPT, in_buf, in_size,
	    out_buf, out_size);
}

/*
 * Default AES cleanup function.
 */
static void crypto_cleanup_aes_openssl(void *context)
{
	struct crypto_ctx_aes_openssl *ctx =
	    (struct crypto_ctx_aes_openssl *)context;

	if (!ctx) {
		return;
	}
	free(ctx);
}

/*
 * Default AES init function: Openssl.
 * The iv parameter MUST point to a buffer with 16 bytes
 * of data to be used as the initialization vector.
 */
static int crypto_init_aes_openssl(struct crypto_state *state,
	const u8 *iv, const u8 *key, size_t key_len)
{
	struct crypto_ctx_aes_openssl *ctx;

	/* Load Openssl common modules */
	crypto_init_openssl();

	ctx = (struct crypto_ctx_aes_openssl *)calloc(1, sizeof(*ctx));
	if (!ctx) {
		log_err("malloc failed");
		goto error;
	}
	if (AES_set_encrypt_key(key, key_len * 8, &ctx->encrypt_key) < 0) {
		log_err("encrypt key init failed");
		goto error;
	}
	if (AES_set_decrypt_key(key, key_len * 8, &ctx->decrypt_key) < 0) {
		log_err("decrypt key init failed");
		goto error;
	}
	memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
	state->encrypt = crypto_encrypt_aes_openssl;
	state->decrypt = crypto_decrypt_aes_openssl;
	state->context_free = crypto_cleanup_aes_openssl;
	state->context = (void *)ctx;
	return 0;
error:
	crypto_cleanup_aes_openssl(ctx);
	return -1;
}

/*
 * Initialize the crypto_state for RSA encryption/decryption with the
 * specified PEM formatted key.
 */
int crypto_init_rsa(struct crypto_state *state,
	enum crypto_rsa_key_type key_type, const char *pem)
{
	ASSERT(state != NULL);

	if (!platform_crypto_init_rsa(state, key_type, pem)) {
		return 0;
	}
	/*
	 * Default to Openssl if platform-specific version failed or
	 * is not implemented.
	 */
	return crypto_init_rsa_openssl(state, key_type, pem);
}

/*
 * Initialize the crypto_state for AES CBC encryption/decryption.
 * The iv parameter MUST point to a buffer with 16 bytes
 * of data to be used as the initialization vector.
 */
int crypto_init_aes(struct crypto_state *state,
	const u8 *iv, const u8 *key, size_t key_len)
{
	ASSERT(state != NULL);
	ASSERT(iv != NULL);
	ASSERT(key != NULL);
	ASSERT(key_len > 0);

	if (!platform_crypto_init_aes(state, iv, key, key_len)) {
		return 0;
	}
	/*
	 * Default to Openssl if platform-specific version failed or
	 * is not implemented.
	 */
	return crypto_init_aes_openssl(state, iv, key, key_len);
}

/*
 * Encrypt the data in in_buf and write it to out_buf.
 * Returns the number of bytes written to out_buf, or -1 on error.
 * If out_buf is NULL, this function returns the maximum number of bytes
 * it would have written, but does nothing.
 */
ssize_t crypto_encrypt(struct crypto_state *state,
	const void *in_buf, size_t in_len,
	void *out_buf, size_t out_size)
{
	ASSERT(state != NULL);
	ASSERT(in_buf != NULL);

	if (!state->encrypt) {
		log_err("encryption not initialized");
		return -1;
	}
	return state->encrypt(state->context, in_buf, in_len,
	    out_buf, out_size);
}

/*
 * Decrypt the data in in_buf and write it to out_buf.
 * Returns the number of bytes written to out_buf, or -1 on error.
 * If out_buf is NULL, this function returns the maximum number of bytes
 * it would have written, but does nothing.
 */
ssize_t crypto_decrypt(struct crypto_state *state,
	const void *in_buf, size_t in_len,
	void *out_buf, size_t out_size)
{
	ASSERT(state != NULL);
	ASSERT(in_buf != NULL);

	if (!state->decrypt) {
		log_err("decryption not initialized");
		return -1;
	}
	return state->decrypt(state->context, in_buf, in_len,
	    out_buf, out_size);
}

/*
 * Free memory and resources associated with the crypto_state.
 */
void crypto_cleanup(struct crypto_state *state)
{
	ASSERT(state != NULL);

	if (!state->context_free) {
		return;
	}
	state->context_free(state->context);
	memset(state, 0, sizeof(*state));
}
