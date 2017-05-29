/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_POLL_EVENT_H__
#define __AYLA_POLL_EVENT_H__

#include <poll.h>

#define POLL_EVENT_NFD	32

struct file_event_table {
	struct pollfd poll[POLL_EVENT_NFD];
	struct file_event_state {
		void (*recv)(void *arg, int fd);
		void (*send)(void *arg, int fd);
		void (*eventf)(void *arg, int fd, int events);
		void *arg;
	} state[POLL_EVENT_NFD];
};

void file_event_init(struct file_event_table *);
int file_event_poll(struct file_event_table *, uint64_t timeout);
int file_event_reg(struct file_event_table *, int fd,
		void (*recv)(void *arg, int fd),
		void (*send)(void *arg, int fd), void *arg);
int file_event_unreg(struct file_event_table *, int fd,
		void (*recv)(void *arg, int fd),
		void (*send)(void *arg, int fd), void *arg);

int file_event_reg_pollf(struct file_event_table *fet, int fd,
		void (*eventf)(void *arg, int fd, int events),
		int events_mask, void *arg);

#endif /* __AYLA_POLL_EVENT_H__ */
