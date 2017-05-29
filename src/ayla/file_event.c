/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <sys/poll.h>
#include <string.h>
#include <stdint.h>

#include <ayla/assert.h>
#include <ayla/file_event.h>
#include <ayla/log.h>

void file_event_init(struct file_event_table *fet)
{
	struct pollfd *pfd;
	int i;

	for (i = 0, pfd = fet->poll; i < POLL_EVENT_NFD; i++, pfd++) {
		pfd->fd = -1;
		pfd->events = 0;
		pfd->revents = 0;
	}
}

static int file_event_find(struct file_event_table *fet, int fd, void *arg)
{
	struct file_event_state *fes;
	struct pollfd *pfd;
	int i;

	/* find matching registration */
	for (i = 0, fes = fet->state, pfd = fet->poll;
	    i < POLL_EVENT_NFD; i++, fes++, pfd++) {
		if (pfd->fd == fd && fes->arg == arg) {
			return i;
		}
	}
	/* otherwise, find empty slot */
	for (i = 0, pfd = fet->poll; i < POLL_EVENT_NFD; i++, pfd++) {
		if (pfd->fd < 0) {
			return i;
		}
	}
	return -1;
}

int file_event_reg(struct file_event_table *fet, int fd,
		void (*recv)(void *arg, int fd),
		void (*send)(void *arg, int fd), void *arg)
{
	struct file_event_state *fes;
	struct pollfd *pfd;
	int i;

	i = file_event_find(fet, fd, arg);
	if (i < 0) {
		log_warn("failed to reg fd %d: file event table full", fd);
		return -1;
	}
	fes = &fet->state[i];
	pfd = &fet->poll[i];

	fes->recv = recv;
	fes->send = send;
	fes->eventf = NULL;
	fes->arg = arg;

	pfd->fd = fd;
	pfd->events = (recv ? (POLLIN | POLLPRI) : 0) | (send ? POLLOUT : 0);
	return 0;
}

int file_event_reg_pollf(struct file_event_table *fet, int fd,
		void (*eventf)(void *arg, int fd, int events),
		int events_mask, void *arg)
{
	struct file_event_state *fes;
	struct pollfd *pfd;
	int i;

	i = file_event_find(fet, fd, arg);
	if (i < 0) {
		log_warn("failed to reg fd %d: file event table full", fd);
		return -1;
	}
	fes = &fet->state[i];
	pfd = &fet->poll[i];

	fes->recv = NULL;
	fes->send = NULL;
	fes->eventf = eventf;
	fes->arg = arg;

	pfd->fd = fd;
	pfd->events = events_mask;
	return 0;
}

int file_event_unreg(struct file_event_table *fet, int fd,
		void (*recv)(void *arg, int fd),
		void (*send)(void *arg, int fd), void *arg)
{
	struct file_event_state *fes;
	struct pollfd *pfd;
	int i;

	i = file_event_find(fet, fd, arg);
	if (i < 0) {
		return -1;
	}
	fes = &fet->state[i];
	pfd = &fet->poll[i];
	if (pfd->fd < 0) {
		return -1;
	}
	pfd->fd = -1;		/* poll() ignores negative fds */
	pfd->events = 0;
	fes->send = NULL;
	fes->recv = NULL;
	fes->eventf = NULL;
	return 0;
}

int file_event_poll(struct file_event_table *fet, uint64_t timeout_ms)
{
	struct file_event_state *fes;
	struct pollfd *pfd;
	short revents;
	int rc;
	int i;

	/* poll() only accepts 32-bit signed integer timeout durations */
	if (timeout_ms > INT32_MAX) {
		timeout_ms = INT32_MAX;
	}
	rc = poll(fet->poll, POLL_EVENT_NFD, (int)timeout_ms);
	if (rc < 0) {
		log_warn("poll failed: %m");
		return -1;
	}
	if (!rc) {
		return 0;	/* Timed out */
	}
	for (i = 0, pfd = fet->poll; i < POLL_EVENT_NFD && rc > 0; i++, pfd++) {
		revents = pfd->revents;
		if (!revents) {
			continue;
		}
		--rc;
		fes = &fet->state[i];
		if ((revents & pfd->events) && fes->eventf) {
			fes->eventf(fes->arg, pfd->fd, revents);
			continue;
		}
		if ((revents & POLLIN) && fes->recv) {
			fes->recv(fes->arg, pfd->fd);
		}
		if ((revents & POLLOUT) && fes->send) {
			fes->send(fes->arg, pfd->fd);
		}
	}
	return rc;	/* File event(s) */
}
