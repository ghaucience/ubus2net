/*
 * Copyright 2016-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#ifndef __AYLA_ASYNC_H__
#define __AYLA_ASYNC_H__

/*
 * Asynchronous operation state.
 */
struct async_op {
	bool active;
	void (*callback)(int, void *);
	void *callback_arg;
	struct timer_head *timers;
	struct timer timer;
	int timeout_result;
};

/*
 * Initialize async_op structure.  Timers parameter is optional, but must
 * be set if a timeout is used.
 */
void async_op_init(struct async_op *op, struct timer_head *timers);

/*
 * Begin an asynchronous operation.  Set timeout to non-zero value
 * if a timeout is desired.  op->timeout_result will be returned if
 * a timeout occurs before the operation is finished.  timeout_result
 * defaults to -1, but may be set to any value by calling
 * async_op_set_timeout_result().
 */
int async_op_start(struct async_op *op,
	void (*callback)(int, void *), void *arg, u64 timeout_ms);

/*
 * Indicate an asynchronous operation has completed, and invoke callback
 * with the result.
 */
int async_op_finish(struct async_op *op, int result);

/*
 * Check if an asynchronous operation is in progress.
 */
static inline bool async_op_active(const struct async_op *op)
{
	return op->active;
}

/*
 * Set a non-default timeout result value.  Default is -1.
 */
void async_op_set_timeout_result(struct async_op *op, int timeout_result);


#endif /* __AYLA_ASYNC_H__ */

