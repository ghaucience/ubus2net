/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_TIMER_H__
#define __AYLA_TIMER_H__

#include "utypes.h"

/*
 * Simple timer structure modified for device client.
 */
struct timer {
	struct timer *next;
	u64 time_ms;	/* monotonic trigger time */
	void (*handler)(struct timer *);
};

struct timer_head {
	struct timer *first;
};

static inline int timer_active(const struct timer *timer)
{
	return timer->time_ms != 0;
}

void timer_init(struct timer *, void (*handler)(struct timer *));
void timer_cancel(struct timer_head *, struct timer *);
void timer_set(struct timer_head *, struct timer *, u64 delay_ms);
void timer_reset(struct timer_head *, struct timer *,
		void (*handler)(struct timer *), u64 delay_ms);
u64 timer_delay_get_ms(struct timer *);

/*
 * Handle timers and return delay until next timer fires.
 * Return -1 if no timers scheduled.
 */
s64 timer_advance(struct timer_head *);

#endif /* __AYLA_TIMER_H__ */
