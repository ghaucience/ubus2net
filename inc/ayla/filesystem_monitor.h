/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#ifndef LIB_AYLA_INCLUDE_AYLA_FILESYSTEM_MONITOR_H_
#define LIB_AYLA_INCLUDE_AYLA_FILESYSTEM_MONITOR_H_

#include <limits.h>
#include <sys/inotify.h>


/*
 * Init state and create an inotify instance.
 *
 * flags can be set to 0 for default behavior, or as follows:
 *   IN_NONBLOCK - filesystem_task() polls for events instead of blocking
 *   IN_CLOEXEC - See the description of the O_CLOEXEC flag in open(2)
 *
 * returns 0 on success, -1 on failure
 */
int fs_monitor_init(int flags);

/*
 * Free resources used by inotify and close the inotify instance
 */
void fs_monitor_cleanup(void);

/*
 * Add a new file watcher.
 * Callback is invoked for each change to a file or directory.
 * Parameters:
 *   path - original path registered to watch
 *   event_mask - mask describing event (see inotify(7))
 *   name - file name, if watching a directory, otherwise NULL
 */
int fs_monitor_add_watcher(const char *path,
	void(*callback)(const char *, uint32_t, const char *),
	uint32_t event_mask);

/*
 * Remove a file watcher
 */
void fs_monitor_del_watcher(const char *path);

/*
 * Call to check for file system events and invoke filesystem_handle_event().
 * Blocks until an event occurs, unless initialized with IN_NONBLOCK option.
 * Returns -1 on error, or 0 if non-blocking and there is nothing to read.
 */
int fs_monitor_task(void);

/*
 * Debug function to print info on all registered watchers
 */
void fs_monitor_print_watchers(void);

#endif /* LIB_AYLA_INCLUDE_AYLA_FILESYSTEM_MONITOR_H_ */
