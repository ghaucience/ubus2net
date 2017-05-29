/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <stdio.h>
#include <ayla/utypes.h>
#include <ayla/time_utils.h>
#include <ayla/timer.h>

void timer_init(struct timer *timer, void (*handler)(struct timer *))
{
	timer->next = NULL;
	timer->time_ms = 0;
	timer->handler = handler;
}

void timer_cancel(struct timer_head *head, struct timer *timer)
{
	struct timer **prev;
	struct timer *node;

	if (!timer_active(timer)) {
		return;
	}
	for (prev = &head->first; (node = *prev) != NULL; prev = &node->next) {
		if (node == timer) {
			*prev = node->next;
			node->time_ms = 0;
			break;
		}
	}
}

void timer_set(struct timer_head *head, struct timer *timer, u64 ms)
{
	struct timer **prev;
	struct timer *node;
	unsigned long long time;

	time = time_mtime_ms() + ms;
	timer_cancel(head, timer);
	timer->time_ms = time;

	for (prev = &head->first; (node = *prev) != NULL; prev = &node->next) {
		if (time < node->time_ms) {
			break;
		}
	}
	*prev = timer;
	timer->next = node;
}

void timer_reset(struct timer_head *head, struct timer *timer,
		void (*handler)(struct timer *), u64 ms)
{
	timer_cancel(head, timer);
	timer_init(timer, handler);
	timer_set(head, timer, ms);
}


u64 timer_delay_get_ms(struct timer *timer)
{
	if (!timer->time_ms) {
		return 0;
	}
	return timer->time_ms - time_mtime_ms();
}

s64 timer_advance(struct timer_head *head)
{
	struct timer *node;
	u64 mtime = 0;

	while ((node = head->first) != NULL) {
		/*
		 * Return if the next timeout is in the future, but re-check
		 * the current time in case the last handler took a while.
		 */
		if (node->time_ms > mtime) {
			mtime = time_mtime_ms();
			if (node->time_ms > mtime) {
				return node->time_ms - mtime;
			}
		}
		head->first = node->next;
		node->time_ms = 0;
		node->next = NULL;
		node->handler(node);
	}
	return -1;
}
