/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include <ayla/log.h>
#include <ayla/assert.h>
#include <ayla/filesystem_monitor.h>

#define INOTIFY_EVENT_MAX_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)
/* queue up to 10 file events */
#define INOTIFY_EVENT_BUF_SIZE (INOTIFY_EVENT_MAX_SIZE * 10)

#define LIST_ITERATE(list, item) \
	for ((item) = (list); (item); (item) = (item)->next)

#define PATH_CHANGE_EVENTS	(IN_DELETE_SELF | IN_MOVE_SELF)

struct filesystem_watcher {
	const char *path;
	uint32_t mask;
	int wd;
	void(*callback)(const char *, uint32_t, const char *);
	struct filesystem_watcher *next;
};

struct filesystem_monitor_state {
	int fd;
	uint8_t *event_buf;
	struct filesystem_watcher *watch_list;
};
static struct filesystem_monitor_state state;


/*
 * Removes the watcher entry and frees memory allocated at watch_ptr.
 * DO NOT use watch_ptr after calling this function.
 */
static void fs_monitor_cleanup_watcher(struct filesystem_watcher *watch_ptr)
{
	if (watch_ptr->wd > 0) {
		inotify_rm_watch(state.fd, watch_ptr->wd);
	}
	free(watch_ptr);
}

/*
 * Removes and deallocates all watcher entries
 */
static void fs_monitor_del_all_watchers(void)
{
	struct filesystem_watcher *watcher;

	while (state.watch_list) {
		watcher = state.watch_list;
		state.watch_list = state.watch_list->next;
		fs_monitor_cleanup_watcher(watcher);
	}
}

/*
 * Init state and create an inotify instance.
 *
 * flags can be set to 0 for default behavior, or as follows:
 *   IN_NONBLOCK - fs_monitor_task() polls for events instead of blocking
 *   IN_CLOEXEC - See the description of the O_CLOEXEC flag in open(2)
 *
 * returns 0 on success, -1 on failure
 */
int fs_monitor_init(int flags)
{
	if (!state.event_buf) {
		state.event_buf = (uint8_t *)malloc(INOTIFY_EVENT_BUF_SIZE);
		if (!state.event_buf) {
			log_err("failed to allocate event buffer: %m");
			goto error;
		}
	}

	if (state.fd <= 0) {
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
		/* inotify_init1() not defined before glibc 2.5 */
		ASSERT(!flags);
		state.fd = inotify_init();
#else
		state.fd = inotify_init1(flags);
#endif
		if (-1 == state.fd) {
			log_err("failed to initialize inotify: %m");
			goto error;
		}
	} else {
		/* remove any existing watchers if the fd was already open */
		fs_monitor_del_all_watchers();
	}

	state.watch_list = NULL;

	return 0;
error:
	fs_monitor_cleanup();
	return -1;
}

/*
 * Free resources used by inotify and close the inotify instance
 */
void fs_monitor_cleanup(void)
{
	if (state.fd > 0) {
		fs_monitor_del_all_watchers();
		close(state.fd);
		state.fd = -1;
	}

	if (state.event_buf) {
		free(state.event_buf);
		state.event_buf = NULL;
	}
}

/*
 * Add a new file watcher
 */
int fs_monitor_add_watcher(const char *path,
	void(*callback)(const char *, uint32_t, const char *),
	uint32_t event_mask)
{
	struct filesystem_watcher *curr;
	struct filesystem_watcher *watcher = NULL;

	if (state.fd <= 0) {
		log_warn("not initialized");
		goto error;
	}

	/* duplicate watchers not supported */
	LIST_ITERATE(state.watch_list, curr) {
		if (!strcmp(path, curr->path)) {
			log_err("duplicate watcher for %s", path);
			goto error;
		}
	}

	watcher = malloc(sizeof(*watcher));
	if (!watcher) {
		log_err("failed to allocate watcher for %s: %m", path);
		goto error;
	}

	watcher->wd = inotify_add_watch(state.fd, path, event_mask |
	    PATH_CHANGE_EVENTS);
	if (watcher->wd == -1) {
		log_err("failed to add watcher for %s: %m", path);
		goto error;
	}
	watcher->path = path;
	watcher->mask = event_mask;
	watcher->callback = callback;

	/* push watcher on watch_list */
	watcher->next = state.watch_list;
	state.watch_list = watcher;
	return 0;
error:
	if (watcher) {
		fs_monitor_cleanup_watcher(watcher);
	}
	return -1;
}

/*
 * Remove a file watcher
 */
void fs_monitor_del_watcher(const char *path)
{
	struct filesystem_watcher *last;
	struct filesystem_watcher *watcher;

	if (state.fd <= 0) {
		log_warn("not initialized");
		return;
	}

	last = NULL;
	LIST_ITERATE(state.watch_list, watcher) {
		if (!strcmp(path, watcher->path)) {
			/* pop watcher off watch_list */
			if (last) {
				last->next = watcher->next;
			} else {
				state.watch_list = watcher->next;
			}

			fs_monitor_cleanup_watcher(watcher);
			break;
		}
		last = watcher;
	}
}

/*
 * Callback invoked when inotify receives a file system event
 */
static void fs_monitor_handle_event(struct inotify_event *event)
{
	struct filesystem_watcher *watcher = NULL;

	LIST_ITERATE(state.watch_list, watcher) {
		if (watcher->wd == event->wd) {
			/* execute callback if it is for a monitored event */
			if (watcher->mask & event->mask) {
				watcher->callback(watcher->path, event->mask,
				    event->len ? event->name : NULL);
			}
			break;
		}
	}

	/*
	 * If the watched file/directory was moved or deleted, or the watcher
	 * was externally removed, attempt to restore it for the watched path.
	 */
	if (watcher && (event->mask & (PATH_CHANGE_EVENTS | IN_IGNORED))) {
		inotify_rm_watch(state.fd, event->wd);
		watcher->wd = inotify_add_watch(state.fd, watcher->path,
		    watcher->mask);

		if (watcher->wd == -1) {
			log_warn("removing invalid watcher %s: %m",
			    watcher->path);
			fs_monitor_del_watcher(watcher->path);
		}
	}
}

/*
 * Call to check for file system events and invoke fs_monitor_handle_event().
 * Blocks until an event occurs, unless initialized with IN_NONBLOCK option.
 * Returns -1 on error, or 0 if non-blocking and there is nothing to read.
 */
int fs_monitor_task(void)
{
	struct inotify_event *event;
	ssize_t len;
	size_t event_size;

	if (!state.event_buf || state.fd <= 0) {
		log_warn("not initialized");
		return -1;
	}

	/* read event data into the event buffer */
	while (state.fd > 0) {
		len = read(state.fd, state.event_buf, INOTIFY_EVENT_BUF_SIZE);

		/* check for error (or no data in non-blocking mode) */
		if (len <= 0) {
			/* EAGAIN or EWOULDBLOCK expected if non-blocking */
			if (len == -1 &&
			    errno != EAGAIN &&
			    errno != EWOULDBLOCK) {
				log_err("failed reading  inotify event: %m");
				return -1;
			}
			/* non-blocking exit point */
			return 0;
		}

		event = (struct inotify_event *)state.event_buf;

		/* Extract event struct(s) from the buffer */
		while (len >= sizeof(*event)) {
			event_size = sizeof(*event) + event->len;

			/* paranoid check for event name truncation */
			if (len < event_size) {
				break;
			}

			fs_monitor_handle_event(event);

			/* Go to to next event */
			len -= event_size;
			event = (struct inotify_event *)((size_t)event +
			    event_size);
		}

		if (len > 0) {
			log_warn("read partial event: discarding %zd bytes",
			    len);
		}
	}

	return 0;
}

void fs_monitor_print_watchers(void)
{
	struct filesystem_watcher *watcher;

	LIST_ITERATE(state.watch_list, watcher) {
		printf("\tWatcher: path=%s event_mask=%08x wd=%d\n",
		    watcher->path, watcher->mask, watcher->wd);
	}
}
