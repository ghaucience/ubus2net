/*
 * Copyright 2016-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_BUFFER_H__
#define __AYLA_BUFFER_H__

#define SUPPORT_JSON

#ifdef SUPPORT_JSON
#include <jansson.h>
#endif

/*
 * Generic buffer structure.
 */
struct buf_data {
	size_t size;
	size_t len;
	u8 buf[0];
};

/*
 * Queue buffer option flags.
 *   QBUF_OPT_PRE_ALLOC: pre-allocates a buffer of at least min_buf_size length
 */
enum queue_buf_opts {
	QBUF_OPT_PRE_ALLOC	= BIT(0),
};

/*
 * Queue buffer linked list element.
 */
struct queue_buf_data {
	struct queue_buf_data *next;
	struct buf_data data;
};

/*
 * Queue buffer state.
 */
struct queue_buf {
	size_t max_len;
	size_t len;
	size_t min_buf_size;
	unsigned opt_mask;
	struct queue_buf_data *first;
	struct queue_buf_data *last;
};

/*
 * Initialize a queue_buf.
 *   opts: bit mask of queue_buf_opts
 *   min_buf_size: Enforced minimum buffer size when allocating new buffer
 *                 segments.  Recommended when many small puts are expected.
 * Opts and min_buf_size may be set to 0 if not needed.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_init(struct queue_buf *qbuf, unsigned opts, size_t min_buf_size);

/*
 * Frees all allocated buffers and resets queue state.
 */
void queue_buf_destroy(struct queue_buf *qbuf);

/*
 * Specify a maximum length for the queue_buf.  With a max_len set,
 * puts or concatenations requiring more than max_len will fail.
 */
void queue_buf_set_max_len(struct queue_buf *qbuf, size_t max_len);

/*
 * Return number of bytes in buffer.
 */
size_t queue_buf_len(const struct queue_buf *qbuf);

/*
 * Clear data and restore queue_buf to its initialized state.
 */
void queue_buf_reset(struct queue_buf *qbuf);

/*
 * Remove data from the end of the queue until its length is new_len.
 * new_len must be less than or equal to qbuf->len.
 */
void queue_buf_trim(struct queue_buf *qbuf, size_t new_len);

/*
 * Remove data from the front of the queue until its length is new_len.
 * new_len must be less than or equal to qbuf->len.
 */
void queue_buf_trim_head(struct queue_buf *qbuf, size_t new_len);

/*
 * Append the data in qbuf2 to the end of qbuf1.  Resets qbuf2.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_concat(struct queue_buf *qbuf1, struct queue_buf *qbuf2);

/*
 * Combines data from all linked buffer elements into the first element,
 * then returns a pointer to the first element's buffer.  This is useful if
 * a contiguous data buffer is needed.  Reallocates the first buffer to fit
 * all data, if necessary.
 */
void *queue_buf_coalesce(struct queue_buf *qbuf);

/*
 * Appends data to the end of the buffer.  If min_buf_size is not set,
 * each put will allocate a new buffer.  Otherwise, each put will fill any
 * empty space in the previous buffer first, then allocate a new buffer of
 * at least min_buf_size to fit the remaining data.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_put(struct queue_buf *qbuf, const void *data, size_t len);

/*
 * Appends a formatted string to the end of the buffer, omitting the null-
 * terminator.  If min_buf_size is not set, each put will allocate a new
 * buffer.  Otherwise, each put will fill any empty space in the previous
 * buffer first, then allocate a new buffer of at least min_buf_size to fit
 * the remaining data.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_putf(struct queue_buf *qbuf, const char *fmt, ...)
	__attribute__  ((format (printf, 2, 3)));

/*
 * Prepends data to the front of the buffer. Each put allocates a new buffer.
 * If min_buf_size is set, each buffer will be at least min_buf_size.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_put_head(struct queue_buf *qbuf, const void *data, size_t len);

/*
 * Copies up to len bytes of data into buf, starting at the specified offset.
 * Returns the number of bytes copied (may be less than len, if the end of
 * the buffer is reached).
 */
size_t queue_buf_copyout(const struct queue_buf *qbuf, void *buf, size_t len,
	size_t offset);

/*
 * Iterates through each buffer and invokes callback for each one.  Callback
 * should return 0 on success, or -1 on error in order to stop iterating.
 * Arg is an argument to pass to the callback.
 */
int queue_buf_walk(const struct queue_buf *qbuf,
	int (*callback)(const void *, size_t, void *), void *arg);

#ifdef SUPPORT_JSON
/*
 * Appends data in JSON format to the end of the buffer.  Puts behave the same
 * as a normal queue_buf_put() call.
 * Returns 0 on success or -1 on failure.
 */
int queue_buf_put_json(struct queue_buf *qbuf, const json_t *json);

/*
 * Parses JSON encoded buffer data starting at the specified offset.
 * Returns a pointer to the JSON structure, or NULL, if parsing failed.
 * Buffer data does not need to be NULL terminated.
 */
json_t *queue_buf_parse_json(const struct queue_buf *qbuf, size_t offset);
#endif

/*
 * Print the state of a queue_buf for debugging purposes.
 */
void queue_buf_dump(const struct queue_buf *qbuf, const char *name,
	bool with_hex);

#endif /* __AYLA_BUFFER_H__ */
