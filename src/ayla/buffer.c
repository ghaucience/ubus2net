/*
 * Copyright 2016-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/assert.h>
#include <ayla/buffer.h>

/* set IO subsystem for all log calls in this file */
#undef log_base
#define log_base(func, level, ...)	\
	log_base_subsystem(func, level, LOG_SUB_IO, __VA_ARGS__)

/*
 * XXX Alternative singly linked queue implementation due to problems
 * with sys/queue.h
 */
#define QBUF_EMPTY(qbuf)	\
	(QBUF_HEAD(qbuf) == NULL)
#define QBUF_HEAD(qbuf)		\
	((qbuf)->first)
#define QBUF_TAIL(qbuf)		\
	((qbuf)->last)
#define QBUF_NEXT(dp)		\
	((dp)->next)
#define QBUF_FOREACH(dp, qbuf)	\
	for (dp = QBUF_HEAD(qbuf); dp; dp = QBUF_NEXT(dp))
#define QBUF_INIT(qbuf)					\
	do {						\
		QBUF_HEAD(qbuf) = NULL;			\
		QBUF_TAIL(qbuf) = NULL;			\
	} while (0)
#define QBUF_REMOVE_HEAD(qbuf)					\
	do {							\
		if (QBUF_EMPTY(qbuf))				\
			break;					\
		QBUF_HEAD(qbuf) = QBUF_NEXT(QBUF_HEAD(qbuf));	\
		if (!QBUF_HEAD(qbuf)) {				\
			QBUF_TAIL(qbuf) = NULL;			\
		}						\
	} while (0)
#define QBUF_REMOVE_NEXT(qbuf, dp)				\
	do {							\
		if (!QBUF_NEXT(dp))				\
			break;					\
		QBUF_NEXT(dp) = QBUF_NEXT(QBUF_NEXT(dp));	\
		if (!QBUF_NEXT(dp)) {				\
			QBUF_TAIL(qbuf) = dp;			\
		}						\
	} while (0)
#define QBUF_REMOVE_ALL_NEXT(qbuf, dp)				\
	do {							\
		if (!QBUF_NEXT(dp))				\
			break;					\
		QBUF_NEXT(dp) = NULL;				\
		QBUF_TAIL(qbuf) = dp;				\
	} while (0)
#define QBUF_INSERT_HEAD(qbuf, dp)				\
	do {							\
		QBUF_NEXT(dp) = QBUF_HEAD(qbuf);		\
		if (QBUF_EMPTY(qbuf)) {				\
			QBUF_TAIL(qbuf) = dp;			\
		}						\
		QBUF_HEAD(qbuf) = dp;				\
	} while (0)
#define QBUF_INSERT_TAIL(qbuf, dp)				\
	do {							\
		QBUF_NEXT(dp) = NULL;				\
		if (QBUF_EMPTY(qbuf)) {				\
			QBUF_HEAD(qbuf) = dp;			\
		} else {					\
			QBUF_NEXT(QBUF_TAIL(qbuf)) = dp;	\
		}						\
		QBUF_TAIL(qbuf) = dp;				\
	} while (0)
#define QBUF_CONCAT(qbuf1, qbuf2)				\
	do {							\
		if (QBUF_EMPTY(qbuf2))				\
			break;					\
		if (QBUF_EMPTY(qbuf1)) {			\
			QBUF_HEAD(qbuf1) = QBUF_HEAD(qbuf2);	\
		} else {					\
			QBUF_NEXT(QBUF_TAIL(qbuf1)) = QBUF_HEAD(qbuf2);	\
		}						\
		QBUF_TAIL(qbuf1) = QBUF_TAIL(qbuf2);		\
		QBUF_INIT(qbuf2);				\
	} while (0)


/*
 * Initialize a generic data buffer.
 */
static void buf_data_init(struct buf_data *dp, size_t size)
{
	dp->size = size;
	dp->len = 0;
}

/*
 * Structure representing an offset in a queue buffer.
 */
struct queue_buf_pos {
	struct queue_buf_data *dp;
	size_t offset;
};


static struct queue_buf_data *queue_buf_data_realloc(struct queue_buf_data *dp,
	size_t size)
{
	struct queue_buf_data *new_dp;

	ASSERT(size > 0);

	if (dp && dp->data.size == size) {
		return dp;
	}
	new_dp = (struct queue_buf_data *)realloc(dp,
	    sizeof(struct queue_buf_data) + size);
	if (!new_dp) {
		log_err("allocation failed");
		return dp;
	}
	if (dp) {
		new_dp->data.size = size;
		if (new_dp->data.len > size) {
			new_dp->data.len = size;
		}
	} else {
		buf_data_init(&new_dp->data, size);
	}
	return new_dp;
}

static inline struct queue_buf_data *queue_buf_data_alloc(size_t size)
{
	return queue_buf_data_realloc(NULL, size);
}

static void queue_buf_data_free_tail(struct queue_buf *qbuf,
	struct queue_buf_data *new_tail)
{
	struct queue_buf_data *dp, *dp2;

	if (new_tail) {
		dp = QBUF_NEXT(new_tail);
		QBUF_REMOVE_ALL_NEXT(qbuf, new_tail);
	} else {
		dp = QBUF_HEAD(qbuf);
		QBUF_INIT(qbuf);
	}
	while (dp) {
		dp2 = QBUF_NEXT(dp);
		free(dp);
		dp = dp2;
	}
}

static int queue_buf_get_pos(const struct queue_buf *qbuf, size_t offset,
	struct queue_buf_pos *pos)
{
	if (qbuf->len <= offset) {
		return -1;
	}
	QBUF_FOREACH(pos->dp, qbuf) {
		if (pos->dp->data.len >= offset) {
			pos->offset = offset;
			return 0;
		}
		offset -= pos->dp->data.len;
	}
	return -1;
}

int queue_buf_init(struct queue_buf *qbuf, unsigned opts, size_t min_buf_size)
{
	ASSERT(qbuf != NULL);
	/* min_buf_size is required for the following options */
	ASSERT(!(opts & QBUF_OPT_PRE_ALLOC) || min_buf_size > 0);

	qbuf->max_len = 0;
	qbuf->min_buf_size = min_buf_size;
	qbuf->opt_mask = opts;
	QBUF_INIT(qbuf);
	queue_buf_reset(qbuf);
	return 0;
}

void queue_buf_destroy(struct queue_buf *qbuf)
{
	ASSERT(qbuf != NULL);

	qbuf->opt_mask = 0;
	queue_buf_reset(qbuf);
}

void queue_buf_set_max_len(struct queue_buf *qbuf, size_t max_len)
{
	ASSERT(qbuf != NULL);

	if (qbuf->len > max_len) {
		queue_buf_trim(qbuf, max_len);
	}
	qbuf->max_len = max_len;
}

size_t queue_buf_len(const struct queue_buf *qbuf)
{
	ASSERT(qbuf != NULL);

	return qbuf->len;
}

void queue_buf_reset(struct queue_buf *qbuf)
{
	struct queue_buf_data *dp = NULL;

	ASSERT(qbuf != NULL);

	qbuf->len = 0;

	/* Retain first buffer if pre-alloc set */
	if (qbuf->opt_mask & QBUF_OPT_PRE_ALLOC) {
		dp = QBUF_HEAD(qbuf);
		if (!dp || dp->data.size != qbuf->min_buf_size) {
			/* Reset to min buf size if changed */
			QBUF_REMOVE_HEAD(qbuf);
			dp = queue_buf_data_realloc(dp, qbuf->min_buf_size);
			if (!dp) {
				/* Failed to allocate first buffer */
				return;
			}
			QBUF_INSERT_HEAD(qbuf, dp);
		}
		dp->data.len = 0;
	}
	/* Free any remaining buffers */
	queue_buf_data_free_tail(qbuf, dp);
}

void queue_buf_trim(struct queue_buf *qbuf, size_t new_len)
{
	struct queue_buf_data *dp;;
	size_t len = 0;

	ASSERT(qbuf != NULL);

	if (new_len >= qbuf->len) {
		return;
	}
	if (!new_len) {
		queue_buf_reset(qbuf);
		return;
	}
	qbuf->len = new_len;

	/* Find the last buffer to retain */
	QBUF_FOREACH(dp, qbuf) {
		len += dp->data.len;
		if (len >= new_len) {
			break;
		}
	}
	ASSERT(dp != NULL);	/* Fails if qbuf->len is incorrect */
	/* Trim buffer len */
	dp->data.len -= len - new_len;
	/* Free any unused trailing buffers */
	queue_buf_data_free_tail(qbuf, dp);
}

void queue_buf_trim_head(struct queue_buf *qbuf, size_t new_len)
{
	struct queue_buf_data *dp, *dp2, *pre_alloc_dp = NULL;
	size_t len = 0;

	ASSERT(qbuf != NULL);

	if (new_len >= qbuf->len) {
		return;
	}
	if (!new_len) {
		queue_buf_reset(qbuf);
		return;
	}
	len = qbuf->len - new_len;	/* len is how many bytes to trim */
	qbuf->len = new_len;

	/* Remove or clear any full buffers to be trimmed */
	dp = QBUF_HEAD(qbuf);
	if ((qbuf->opt_mask & QBUF_OPT_PRE_ALLOC) && dp->data.len <= len) {
		len -= dp->data.len;
		dp->data.len = 0;
		pre_alloc_dp = dp;
		dp = QBUF_NEXT(dp);
	}
	while (dp && dp->data.len <= len) {
		len -= dp->data.len;
		if (pre_alloc_dp) {
			QBUF_REMOVE_NEXT(qbuf, pre_alloc_dp);
			dp2 = QBUF_NEXT(pre_alloc_dp);
		} else {
			QBUF_REMOVE_HEAD(qbuf);
			dp2 = QBUF_HEAD(qbuf);
		}
		free(dp);
		dp = dp2;
	}
	if (!len && !pre_alloc_dp) {
		return;
	}
	/* Align partial buffers */
	dp = QBUF_HEAD(qbuf);
	/* Special case to handle an empty fixed first buffer */
	if (!dp->data.len) {
		dp2 =  QBUF_NEXT(dp);
		ASSERT(dp2 != NULL);	/* Should only be here if next exists */
		/* Copy data from second buffer to fixed first buffer */
		dp->data.len = dp2->data.len - len;
		if (dp->data.len > dp->data.size) {
			dp->data.len = dp->data.size;
		}
		len += dp->data.len;
		memcpy(dp->data.buf, dp2->data.buf, dp->data.len);
		if (len == dp2->data.len) {
			/* Moved all data from next node to first node */
			QBUF_REMOVE_NEXT(qbuf, dp);
			free(dp2);
			return;
		}
		dp = dp2;
	}
	/* Standard case to shift remaining data to front of buffer */
	memmove(dp->data.buf, dp->data.buf + len, dp->data.len - len);
	dp->data.len -= len;
}

int queue_buf_concat(struct queue_buf *qbuf1, struct queue_buf *qbuf2)
{
	ASSERT(qbuf1 != NULL);
	ASSERT(qbuf2 != NULL);

	if (qbuf1->max_len && qbuf1->max_len < qbuf1->len + qbuf2->len) {
		log_err("exceeds max size: %zu bytes", qbuf1->max_len);
		return -1;
	}
	qbuf1->len += qbuf2->len;
	QBUF_CONCAT(qbuf1, qbuf2);
	queue_buf_reset(qbuf2);
	return 0;
}

void *queue_buf_coalesce(struct queue_buf *qbuf)
{
	struct queue_buf_data *first, *dp;

	ASSERT(qbuf != NULL);

	first = QBUF_HEAD(qbuf);
	/* No data */
	if (!first || !qbuf->len) {
		return NULL;
	}
	/* All data in first buffer */
	if (first->data.len == qbuf->len) {
		return first->data.buf;
	}
	/* Expand first buffer to fit all data */
	if (qbuf->len > first->data.size) {
		QBUF_REMOVE_HEAD(qbuf);
		first = queue_buf_data_realloc(first, qbuf->len);
		QBUF_INSERT_HEAD(qbuf, first);
		if (first->data.size != qbuf->len) {
			return NULL;
		}
	}
	/* Copy all data into first buffer and delete trailing buffers */
	for (dp = QBUF_NEXT(first); dp; dp = QBUF_NEXT(first)) {
		ASSERT(first->data.size >= first->data.len + dp->data.len);
		memcpy(first->data.buf + first->data.len, dp->data.buf,
		    dp->data.len);
		first->data.len += dp->data.len;
		QBUF_REMOVE_NEXT(qbuf, first);
		free(dp);
	}
	return first->data.buf;
}

int queue_buf_put(struct queue_buf *qbuf, const void *data, size_t len)
{
	struct queue_buf_data *dp;
	size_t tlen;

	ASSERT(qbuf != NULL);
	ASSERT(data != NULL);

	if (!len) {
		return 0;
	}
	if (qbuf->max_len && qbuf->max_len < qbuf->len + len) {
		log_err("exceeds max size: %zu bytes", qbuf->max_len);
		return -1;
	}
	dp = QBUF_TAIL(qbuf);
	if (!dp) {
		goto fill_new;
	}
	/* Fill trailing buffer, if it is not full */
	tlen = dp->data.size - dp->data.len;
	if (!tlen) {
		goto fill_new;
	}
	if (tlen > len) {
		tlen = len;
	}
	memcpy(dp->data.buf + dp->data.len, data, tlen);
	dp->data.len += tlen;
	qbuf->len += tlen;
	data = (u8 *)data + tlen;
	len -= tlen;
	if (!len) {
		return 0;
	}
fill_new:
	/* Add a new buffer to the tail */
	dp = queue_buf_data_alloc(len < qbuf->min_buf_size ?
	    qbuf->min_buf_size : len);
	if (!dp) {
		log_err("allocation failed");
		return -1;
	}
	memcpy(dp->data.buf, data, len);
	dp->data.len = len;
	qbuf->len += len;
	QBUF_INSERT_TAIL(qbuf, dp);
	return 0;
}

int queue_buf_putf(struct queue_buf *qbuf, const char *fmt, ...)
{
	va_list args;
	char *data;
	int rc;

	va_start(args, fmt);
	rc = vasprintf(&data, fmt, args);
	va_end(args);
	if (rc < 0) {
		return -1;
	}
	rc = queue_buf_put(qbuf, data, rc);
	free(data);
	return rc;
}

int queue_buf_put_head(struct queue_buf *qbuf, const void *data, size_t len)
{
	struct queue_buf_data *dp;

	ASSERT(qbuf != NULL);
	ASSERT(data != NULL);

	dp = queue_buf_data_alloc(len < qbuf->min_buf_size ?
	    qbuf->min_buf_size : len);
	if (!dp) {
		log_err("allocation failed");
		return -1;
	}
	memcpy(dp->data.buf, data, len);
	dp->data.len = len;
	qbuf->len += len;
	QBUF_INSERT_HEAD(qbuf, dp);
	return 0;
}

static size_t queue_buf_copyout_pos(struct queue_buf_pos *pos,
	void *buf, size_t len)
{
	size_t tlen;
	size_t nbytes = len;

	while (pos->dp && len) {
		/* Sanity check offset */
		ASSERT(pos->offset <= pos->dp->data.len);

		tlen = pos->dp->data.len - pos->offset;
		if (!tlen) {
			pos->dp = QBUF_NEXT(pos->dp);
			pos->offset = 0;
			continue;
		}
		if (tlen > len) {
			tlen = len;
		}
		memcpy(buf, pos->dp->data.buf + pos->offset, tlen);
		buf += tlen;
		len -= tlen;
		pos->offset += tlen;
	}
	return nbytes - len;
}

size_t queue_buf_copyout(const struct queue_buf *qbuf, void *buf, size_t len,
	size_t offset)
{
	struct queue_buf_pos pos;

	ASSERT(qbuf != NULL);
	ASSERT(buf != NULL);

	if (queue_buf_get_pos(qbuf, offset, &pos) < 0) {
		log_err("no data to copy");
		return 0;
	}
	return queue_buf_copyout_pos(&pos, buf, len);
}

int queue_buf_walk(const struct queue_buf *qbuf,
	int (*callback)(const void *, size_t, void *), void *arg)
{
	struct queue_buf_data *dp;

	ASSERT(qbuf != NULL);
	ASSERT(callback != NULL);

	QBUF_FOREACH(dp, qbuf) {
		if (callback(dp->data.buf, dp->data.len, arg) < 0) {
			return -1;
		}
	}
	return 0;
}

#ifdef SUPPORT_JSON
static int queue_buf_json_write(const char *buf, size_t len, void *arg)
{
	struct queue_buf *qbuf = (struct queue_buf *)arg;

	return queue_buf_put(qbuf, buf, len);
}

static size_t queue_buf_json_read(void *buf, size_t len, void *arg)
{
	struct queue_buf_pos *pos = (struct queue_buf_pos *)arg;

	return queue_buf_copyout_pos(pos, buf, len);
}

int queue_buf_put_json(struct queue_buf *qbuf, const json_t *json)
{
	const size_t JSON_MIN_BUF_SIZE = 1024;
	int rc;
	size_t orig_min_buf_size;

	ASSERT(qbuf != NULL);
	ASSERT(json != NULL);
	orig_min_buf_size = qbuf->min_buf_size;

	/*
	 * json_dump_callback() may do many small puts, so force a minimum
	 * buffer size for improved performance.
	 */
	if (orig_min_buf_size < JSON_MIN_BUF_SIZE) {
		qbuf->min_buf_size = JSON_MIN_BUF_SIZE;
	}
	rc = json_dump_callback(json, queue_buf_json_write, qbuf,
	    JSON_COMPACT);
	qbuf->min_buf_size = orig_min_buf_size;
	return rc;
}

json_t *queue_buf_parse_json(const struct queue_buf *qbuf, size_t offset)
{
	struct queue_buf_pos pos;
	json_error_t error;
	json_t *root;

	ASSERT(qbuf != NULL);

	if (queue_buf_get_pos(qbuf, offset, &pos) < 0) {
		log_err("no data to copy");
		return NULL;
	}
	root = json_load_callback(queue_buf_json_read, &pos, 0, &error);
	if (!root) {
		log_err("JSON parse error at line %d: %s",
		    error.line, error.text);
	}
	return root;
}
#endif

void queue_buf_dump(const struct queue_buf *qbuf, const char *name,
	bool with_hex)
{
	struct queue_buf_data *dp;
	unsigned i = 0;

	ASSERT(qbuf != NULL);

	log_debug("%s:\t max_len=%zu min_buf_size=%zu len=%zu opts=%04X",
	    name ? name : "queue",
	    qbuf->max_len, qbuf->min_buf_size, qbuf->len, qbuf->opt_mask);
	QBUF_FOREACH(dp, qbuf) {
		log_debug("data[%02u]\t size=%zu len=%zu",
		    i++, dp->data.size, dp->data.len);
		if (with_hex && dp->data.len) {
			log_debug_hex("buf\t", dp->data.buf, dp->data.len);
		}
	}
}
