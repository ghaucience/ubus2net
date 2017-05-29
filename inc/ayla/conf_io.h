/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_CONF_IO_H__
#define __AYLA_CONF_IO_H__

#include <jansson.h>

#define CONF_STARTUP_FILE_EXT	"startup"

/*
 * Set flag enabling loading and saving of config directly from/to factory
 * config.  This should ONLY be used for factory setup, and requires the
 * factory config file to be writable.
 */
void conf_factory_edit_mode_enable(void);

/*
 * Initialize config and allocate required resources.  This MUST be called
 * prior to invoking other conf functions.  The factory_conf_file is the
 * full path to the factory default config file.  The startup_dir is the path
 * of the startup config directory.  Startup_dir is optional, and may be set
 * to NULL.  Without a startup_dir specified, the startup config file is
 * written in the same directory as the factory config.
 */
int conf_init(const char *factory_conf_file, const char *startup_dir);

/*
 * Free all resources associated with this config instance.
 */
void conf_cleanup(void);

/*
 * Set the get and/or set handlers for the specified config item.
 */
int conf_register(const char *name,
	int (*set)(json_t *), json_t *(*get)(void));

/*
 * Remove the get and/or get handlers for a given config item.
 */
int conf_unregister(const char *name);

/*
 * Apply changes in the root JSON structure by calling all defined set handlers.
 */
int conf_apply(void);

/*
 * Update the root JSON structure by calling all defined get handlers.
 * Note: if there are outstanding changes to the JSON structure, calling
 * conf_update() before calling conf_apply() may overwrite the changes.
 */
int conf_update(void);

/*
 * Load config from the file.  If no startup config is available, read the
 * factory default config file.  Once the config is loaded, the changes are
 * applied using defined set handlers.
 */
int conf_load(void);

/*
 * Save the current config state to the startup config file.  The startup
 * config stores the current working state of configuration, while the
 * factory config file is maintained as read-only.  If get handlers are defined,
 * they are called to update the config state prior to saving.
 */
int conf_save(void);

/*
 * Save a backup of the current config state to a file.
 */
int conf_save_backup(const char *path);

/*
 * Write an empty configuration file.  This is useful if no factory default
 * config file is available.
 */
int conf_save_empty(const char *path);

/*
 * Revert to the factory default config.
 */
int conf_factory_reset(void);

/*
 * Set a new JSON object for a config object.  Call conf_apply() when all sets
 * are complete, to apply the changes to the system.
 * Returns -1 on failure, 0 if the value changed, and 1 if the value matched
 * the existing value.
 */
int conf_set(const char *path, json_t *obj);

/*
 * Identical to conf_set(), but consumes the reference to obj, rather than
 * incrementing its reference counter.
 */
int conf_set_new(const char *path, json_t *obj);

/*
 * Delete the specified config object.  Call conf_apply() when all sets/deletes
 * are complete, to apply the changes to the system.
 */
int conf_delete(const char *path);

/*
 * Return a the JSON object for the specified config object.  Returns NULL if no
 * object with that name exists.  Call conf_update() before calling conf_get()
 * if get handlers are defined, and there might have been changes to the
 * application state that affect config.
 *
 * NOTE: The JSON object returned borrows a reference from the cached config
 * object, so its reference counter should be incremented if a reference to
 * it is stored for later use.
 */
json_t *conf_get(const char *path);

/*
 * Return the startup file path.  Config should be initialized prior to calling
 * this function.
 */
const char *conf_startup_file_path(void);

/*
 * Return true if the last config file loaded was the factory file.
 * Returns false once a startup config file is successfully saved.
 * Unsaved changes do not affect the return value.
 */
bool conf_factory_loaded(void);

/*
 * Register a config change callback.  This will be called whenever a config
 * object changed.  The change_callback may be invoked at a higher level in the
 * tree than the actual change, so handlers should filter on the paths they
 * need.
 */
void conf_set_change_callback(void (*callback)(const char *, const json_t *));


#endif /* __AYLA_CONF_IO_H__ */
