/*
 * Copyright 2016-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdlib.h>

#include <ayla/utypes.h>
#include <ayla/assert.h>
#include <ayla/timer.h>
#include <ayla/async.h>

#define ASYNC_TIMEOUT_RESULT_DEFAULT	-1

/*
 * Handler for an asynchronous event timeout.
 */
static void async_op_timeout(struct timer *timer)
{
	struct async_op *op = CONTAINER_OF(
	    struct async_op, timer, timer);

	async_op_finish(op, op->timeout_result);
}

/*
 * Initialize async_op structure.  Timers parameter is optional, but must
 * be set if a timeout is used.
 */
void async_op_init(struct async_op *op, struct timer_head *timers)
{
	ASSERT(op != NULL);

	op->active = false;
	op->callback = NULL;
	op->callback_arg = NULL;
	op->timers = timers;
	timer_init(&op->timer, async_op_timeout);
	op->timeout_result = ASYNC_TIMEOUT_RESULT_DEFAULT;
}

/*
 * Begin an asynchronous operation.  Set timeout to non-zero value
 * if a timeout is desired.  op->timeout_result will be returned if
 * a timeout occurs before the operation is finished.  timeout_result
 * defaults to -1, but may be set to any value by calling
 * async_op_set_timeout_result().
 */
int async_op_start(struct async_op *op,
	void (*callback)(int, void *), void *arg, u64 timeout_ms)
{
	ASSERT(op != NULL);
	ASSERT(callback != NULL);

	if (op->active) {
		return -1;
	}
	op->active = true;
	op->callback = callback;
	op->callback_arg = arg;
	if (timeout_ms) {
		ASSERT(op->timers != NULL);
		timer_set(op->timers, &op->timer, timeout_ms);
	}
	return 0;
}

/*
 * Indicate an asynchronous operation has completed, and invoke callback
 * with the result.
 */
int async_op_finish(struct async_op *op, int result)
{
	void (*callback)(int, void *);
	void *arg;

	ASSERT(op != NULL);

	if (!op->active) {
		return -1;
	}
	if (op->timers) {
		timer_cancel(op->timers, &op->timer);
	}
	op->active = false;
	if (op->callback) {
		/* Backup callback, in case callback modifies the op */
		callback = op->callback;
		arg = op->callback_arg;
		op->callback = NULL;
		op->callback_arg = NULL;
		callback(result, arg);
	}
	return 0;
}

/*
 * Set a non-default timeout result value.  Default is -1.
 */
void async_op_set_timeout_result(struct async_op *op, int timeout_result)
{
	ASSERT(op != NULL);

	op->timeout_result = timeout_result;
}
