/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <sys/queue.h>

#include <ayla/utypes.h>
#include <ayla/log.h>
#include <ayla/json_parser.h>
#include <ayla/file_io.h>
#include <ayla/assert.h>
#include <ayla/conf_io.h>

/* set CONF subsystem for all log calls in this file */
#undef log_base
#define log_base(func, level, ...)	\
	log_base_subsystem(func, level, LOG_SUB_CONF, __VA_ARGS__)

struct conf_ops {
	char *name;			/* name of subtree object */
	json_t *(*get)(void);		/* return JSON tree for the subsystem */
	int (*set)(json_t *);		/* set subsystem based on object */
	SLIST_ENTRY(conf_ops) list_entry; /* linked list navigation */
};

struct conf_state {
	json_t *root;				/* config JSON object */
	char *factory_file;			/* factory file path */
	char *startup_file;			/* startup file path */
	SLIST_HEAD(, conf_ops) ops_list;	/* list of handlers */
	bool factory_edit_mode;			/* factory file edit flag */
	void (*change_handler)(const char *, const json_t *); /* set callback */
	bool factory_file_loaded;		/* factory file loaded flag */
};

/* Currently, store one config instance per process */
static struct conf_state conf_state;


/*
 * Delete a node in the configuration tree and invoke the change handler.
 */
static int conf_remove_obj(const char *path, json_t *parent, const char *name)
{
	struct conf_state *conf = &conf_state;

	if (json_object_del(parent, name) < 0) {
		return -1;
	}
	if (conf->change_handler) {
		conf->change_handler(path, NULL);
	}
	return 0;
}

/*
 * Replace a node in the configuration tree and invoke the change handler.
 * Does nothing if the new and old nodes are equivalent.  Returns 0 if node
 * changed.
 */
static int conf_set_obj(const char *path, json_t *parent, const char *name,
	json_t *old, json_t *new)
{
	struct conf_state *conf = &conf_state;

	if (old && json_equal(old, new)) {
		return 1;
	}
	if (json_object_set(parent, name, new) < 0) {
		return -1;
	}
	if (conf->change_handler) {
		conf->change_handler(path, new);
	}
	return 0;
}

/*
 * Replace the root node of the configuration tree and invoke the
 * change handler for each sub-node with changes.
 * Does nothing if the new and old nodes are equivalent.
 */
static int conf_set_root_obj(json_t **root_ptr, json_t *new)
{
	struct conf_state *conf = &conf_state;
	const char *key;
	json_t *old;
	json_t *subobj_old, *subobj;

	/* Replace root with new root prior to invoking change handlers */
	old = *root_ptr;
	*root_ptr = new;

	if (new) {
		json_incref(new);
		/* Determine when to call the change handler */
		if (old && conf->change_handler) {
			json_object_foreach(new, key, subobj) {
				subobj_old = json_object_get(old, key);
				if (!subobj_old) {
					/* Sub-object added */
					conf->change_handler(key, subobj);
					continue;
				}
				/* Sub-object updated */
				if (!json_equal(subobj, subobj_old)) {
					conf->change_handler(key, subobj);
				}
				/*
				 * Remove intersecting sub-objects
				 * from old to determine which
				 * sub-objects were deleted.
				 */
				json_object_del(old, key);
			}
		}
	}
	if (old) {
		if (conf->change_handler) {
			json_object_foreach(old, key, subobj_old) {
				/* Sub-object deleted */
				conf->change_handler(key, NULL);
			}
		}
		/* Clear before decref to invalidate any other references */
		json_object_clear(old);
		json_decref(old);
	}
	return 0;
}

/*
 * Read a JSON config file.  The top-level "config" object must be present.
 */
static json_t *conf_read(const char *file)
{
	FILE *fp;
	json_t *top;
	json_t *root;
	json_error_t jerr;

	fp = fopen(file, "r");
	if (!fp) {
		return NULL;
	}

	top = json_loadf(fp, 0, &jerr);
	fclose(fp);
	if (!top) {
		log_err("file %s config parse failed: "
		    "line %d col %d: %s",
		    file, jerr.line, jerr.column, jerr.text);
		return NULL;
	}
	root = json_object_get(top, "config");
	if (root) {
		json_incref(root);
		log_info("read configuration from file: %s", file);

	} else {
		log_err("missing top-level \"config\" object: %s", file);
	}
	json_decref(top);
	return root;
}

/*
 * Write the root config JSON object to a file.  The top-level "config"
 * object will be added.
 */
static int conf_write(json_t *root, const char *file)
{
	FILE *fp;
	json_t *top;
	char temp[strlen(file) + 20];
	int fd;
	int rc = -1;

	snprintf(temp, sizeof(temp), "%sXXXXXX", file);

	fd = mkstemp(temp);
	if (fd < 0) {
		log_err("open of %s failed %m", temp);
		return -1;
	}
	fp = fdopen(fd, "w");
	if (!fp) {
		log_err("fdopen of %s failed %m", temp);
		close(fd);
		return -1;
	}
	top = json_object();
	json_object_set(top, "config", root);
#ifdef BUILD_RELEASE
	/* On release builds, save startup config in more compact format */
	if (conf_state.factory_edit_mode) {
		rc = json_dumpf(top, fp, JSON_INDENT(4));
	} else {
		rc = json_dumpf(top, fp, JSON_COMPACT);
	}
#else
	rc = json_dumpf(top, fp, JSON_INDENT(4));
#endif
	json_decref(top);
	fclose(fp);
	if (rc) {
		log_err("file %s write failed", temp);
		goto unlink;
	}
	rc = rename(temp, file);
	if (rc) {
		log_err("rename %s to %s failed: %m", temp, file);
		goto unlink;
	}
	log_info("wrote configuration to file: %s", file);

unlink:
	unlink(temp);
	return rc;
}

/*
 * Free memory allocated for a conf_ops structure.
 */
static void conf_ops_free(struct conf_ops *ops)
{
	if (!ops) {
		return;
	}
	free(ops->name);
	free(ops);
}

/*
 * Drill down JSON tree to the '/' delimited path specified,
 * starting at root_node.
 * Returns a pointer to the specified node, or NULL if the node does not exist.
 */
static json_t *conf_path_lookup(json_t *root_node, const char *path)
{
	char *path_buf = NULL;
	char *save_ptr;
	char *name;
	json_t *node = NULL;

	path_buf = strdup(path);
	if (!path_buf) {
		log_err("malloc failed");
		goto done;
	}
	name = strtok_r(path_buf, "/", &save_ptr);
	node = root_node;
	do {
		node = json_object_get(node, name);
		if (!node) {
			goto done;
		}
	} while ((name = strtok_r(NULL, "/", &save_ptr)) != NULL);
done:
	free(path_buf);
	return node;
}

/*
 * Drill down JSON tree to the '/' delimited path specified,
 * starting at root_node, and create any missing nodes in the path.  If obj is
 * non-NULL, create or set the object pointed to by the path.  If obj is NULL,
 * delete the object.
 * Returns 0 on success, or -1 on failure.
 */
static int conf_path_set(struct conf_state *conf, const char *path, json_t *obj)
{
	char *path_buf = NULL;
	char *save_ptr;
	char *name;
	char *child_name;
	json_t *parent;
	json_t *node;
	json_t *old_obj;
	int rc = 0;

	path_buf = strdup(path);
	if (!path_buf) {
		log_err("malloc failed");
		rc = -1;
		goto error;
	}
	name = strtok_r(path_buf, "/", &save_ptr);
	parent = conf->root;
	while ((child_name = strtok_r(NULL, "/", &save_ptr)) != NULL) {
		node = json_object_get(parent, name);
		if (!node) {
			/* Create intermediate nodes in path, if needed */
			node = json_object();
			if (json_object_set_new(parent, name, node) < 0) {
				log_err("failed to create %s node in path %s",
				    name, path);
				rc = -1;
				goto error;
			}
		} else if (!json_is_object(node)) {
			log_err("node %s in path %s is not object", name, path);
			rc = -1;
			goto error;
		}
		parent = node;
		name = child_name;
	}
	if (rc < 0) {
		goto error;
	}
	if (obj) {
		/*
		 * Perform type check if overwriting existing node.
		 * Note: Jansson true and false are different types.
		 */
		old_obj = json_object_get(parent, name);
		if (old_obj && json_typeof(old_obj) != json_typeof(obj) &&
		    !(json_is_boolean(old_obj) && json_is_boolean(obj))) {
			log_err("cannot overwrite node %s with different type",
			    path);
			rc = -1;
			goto error;
		}
		/* Set object */
		rc = conf_set_obj(path, parent, name, old_obj, obj);
	} else {
		/* Delete object */
		rc = conf_remove_obj(path, parent, name);
	}
error:
	free(path_buf);
	return rc;
}

/*
 * Set flag enabling loading and saving of config directly from/to factory
 * config.  This should ONLY be used for factory setup, and requires the
 * factory config file to be writable.
 */
void conf_factory_edit_mode_enable(void)
{
	conf_state.factory_edit_mode = true;
}

/*
 * Initialize config and allocate required resources.  This MUST be called
 * prior to invoking other conf functions.  The factory_conf_file is the
 * full path to the factory default config file.  The startup_dir is the path
 * of the startup config directory.  Startup_dir is optional, and may be set
 * to NULL.  Without a startup_dir specified, the startup config file is
 * written in the same directory as the factory config.
 */
int conf_init(const char *factory_conf_file, const char *startup_dir)
{
	struct conf_state *conf = &conf_state;
	char path[PATH_MAX];
	const char *name;

	ASSERT(factory_conf_file != NULL);

	/* Factory config is required */
	if (access(factory_conf_file, R_OK) < 0) {
		log_err("cannot access factory config %s: %m",
		    factory_conf_file);
		goto error;
	}
	conf->factory_file = strdup(factory_conf_file);
	if (!conf->factory_file) {
		log_err("malloc failed");
		goto error;
	}
	/* Generate startup (writable) config file path */
	if (startup_dir) {
		if (!file_is_dir(startup_dir)) {
			log_err("invalid startup dir %s: Not a directory",
			    startup_dir);
			goto error;
		}
		name = file_get_name(conf->factory_file);
		REQUIRE(name != NULL, "invalid factory conf file name");
		snprintf(path, sizeof(path), "%s/%s.%s", startup_dir, name,
		    CONF_STARTUP_FILE_EXT);
	} else {
		snprintf(path, sizeof(path), "%s.%s", conf->factory_file,
		    CONF_STARTUP_FILE_EXT);
	}
	conf->startup_file = strdup(path);
	if (!conf->startup_file) {
		log_err("malloc failed");
		goto error;
	}
	conf->root = json_object();
	SLIST_INIT(&conf->ops_list);
	return 0;
error:
	conf_cleanup();
	return -1;
}

/*
 * Free all resources associated with this config instance.
 */
void conf_cleanup(void)
{
	struct conf_state *conf = &conf_state;
	struct conf_ops *ops;

	/* Cleanup allocated resources */
	if (conf->root) {
		json_decref(conf->root);
	}
	free(conf->factory_file);
	free(conf->startup_file);
	while ((ops = SLIST_FIRST(&conf->ops_list)) != NULL) {
		SLIST_REMOVE_HEAD(&conf->ops_list, list_entry);
		conf_ops_free(ops);
	}
	memset(conf, 0, sizeof(*conf));
}

/*
 * Set the get and/or set handlers for the specified config item.
 */
int conf_register(const char *name,
	int (*set)(json_t *), json_t *(*get)(void))
{
	struct conf_state *conf = &conf_state;
	struct conf_ops *ops;

	ASSERT(name != NULL);
	ASSERT(get != NULL || set != NULL);

	if (!conf->root) {
		log_err("config not initialized");
		return -1;
	}

	ops = (struct conf_ops *)malloc(sizeof(struct conf_ops));
	if (!ops) {
		log_err("malloc failed");
		return -1;
	}
	ops->name = strdup(name);
	ops->get = get;
	ops->set = set;
	SLIST_INSERT_HEAD(&conf->ops_list, ops, list_entry);
	return 0;
}

/*
 * Remove the get and/or get handlers for a given config item.
 */
int conf_unregister(const char *name)
{
	struct conf_state *conf = &conf_state;
	struct conf_ops *ops = NULL;

	ASSERT(name != NULL);

	SLIST_FOREACH(ops, &conf->ops_list, list_entry) {
		if (!strcmp(name, ops->name)) {
			break;
		}
	}
	if (!ops) {
		return -1;
	}
	SLIST_REMOVE(&conf->ops_list, ops, conf_ops, list_entry);
	conf_ops_free(ops);
	return 0;
}

/*
 * Apply changes in the root JSON structure by calling all defined set handlers.
 */
int conf_apply(void)
{
	struct conf_state *conf = &conf_state;
	struct conf_ops *ops;
	json_t *obj;
	int rc = 0;

	SLIST_FOREACH(ops, &conf->ops_list, list_entry) {
		if (!ops->set) {
			continue;
		}
		obj = json_object_get(conf->root, ops->name);
		if (!obj) {
			continue;
		}
		if (ops->set(obj) < 0) {
			log_err("failed to apply %s config", ops->name);
			rc = -1;
		}
	}
	return rc;
}

/*
 * Update the root JSON structure by calling all defined get handlers.
 * Note: if there are outstanding changes to the JSON structure, calling
 * conf_update() before calling conf_apply() may overwrite the changes.
 */
int conf_update(void)
{
	struct conf_state *conf = &conf_state;
	struct conf_ops *ops;
	json_t *obj;
	json_t *old_obj;
	int rc = 0;

	SLIST_FOREACH(ops, &conf->ops_list, list_entry) {
		if (!ops->get) {
			continue;
		}
		obj = ops->get();
		if (!obj) {
			log_err("failed to load %s config", ops->name);
			rc = -1;
			continue;
		}
		old_obj = json_object_get(conf->root, ops->name);
		conf_set_obj(ops->name, conf->root, ops->name,
		    old_obj, obj);
		json_decref(obj);
	}
	return rc;
}

/*
 * Load config from the file.  If no startup config is available, read the
 * factory default config file.
 */
int conf_load(void)
{
	struct conf_state *conf = &conf_state;
	json_t *root;

	if (!conf->root) {
		log_err("config not initialized");
		return -1;
	}

	/* Factory edit mode allows read/write of factory config */
	if (conf->factory_edit_mode) {
		log_info("factory config edit mode enabled");
		goto load_factory;
	}

	/* Normally, attempt to load the startup config first */
	root = conf_read(conf->startup_file);
	if (root) {
		conf->factory_file_loaded = false;
	} else {
		log_debug("no valid startup config: %s", conf->startup_file);
load_factory:
		root = conf_read(conf->factory_file);
		if (root) {
			conf->factory_file_loaded = true;
		} else {
			log_err("failed to load factory config: %s",
			    conf->factory_file);
			return -1;
		}
	}
	if (conf_set_root_obj(&conf->root, root) < 0) {
		return -1;
	}
	json_decref(root);
	if (conf_apply() < 0) {
		return -1;
	}
	return 0;
}

/*
 * Save the current config state to the startup config file.  The startup
 * config stores the current working state of configuration, while the
 * factory config file is maintained as read-only.
 */
int conf_save(void)
{
	struct conf_state *conf = &conf_state;

	/* Update JSON object if get handlers are defined */
	conf_update();

	/* Factory edit mode allows read/write of factory config */
	if (conf->factory_edit_mode) {
		return conf_write(conf->root, conf->factory_file);
	}

	/* Normally, only save to startup config */
	if (conf_write(conf->root, conf->startup_file) < 0) {
		return -1;
	}
	conf->factory_file_loaded = false;
	return 0;
}

/*
 * Save a backup of the current config state to a file.
 */
int conf_save_backup(const char *path)
{
	struct conf_state *conf = &conf_state;

	ASSERT(path != NULL);

	/* Update JSON object if get handlers are defined */
	conf_update();

	return conf_write(conf->root, path);
}

/*
 * Write an empty configuration file.  This is useful if no factory default
 * config file is available.
 */
int conf_save_empty(const char *path)
{
	json_t *root;
	int rc;

	ASSERT(path != NULL);

	root = json_object();
	rc = conf_write(root, path);
	json_decref(root);
	return rc;
}

/*
 * Revert to the factory default config.
 */
int conf_factory_reset(void)
{
	struct conf_state *conf = &conf_state;

	if (conf->factory_edit_mode) {
		log_err("factory reset disabled in factory edit mode");
		return -1;
	}

	unlink(conf->startup_file);
	return conf_load();
}

/*
 * Set a new JSON object for a config object.  Call conf_apply() when all sets
 * are complete, to apply the changes to the system.
 * Returns -1 on failure, 0 if the value changed, and 1 if the value matched
 * the existing value.
 */
int conf_set(const char *path, json_t *obj)
{
	struct conf_state *conf = &conf_state;

	ASSERT(path != NULL);
	ASSERT(obj != NULL);

	return conf_path_set(conf, path, obj);
}

/*
 * Identical to conf_set(), but consumes the reference to obj, rather than
 * incrementing its reference counter.
 */
int conf_set_new(const char *path, json_t *obj)
{
	int rc;

	rc = conf_set(path, obj);
	json_decref(obj);
	return rc;
}

/*
 * Delete the specified config object.  Call conf_apply() when all sets/deletes
 * are complete, to apply the changes to the system.
 */
int conf_delete(const char *path)
{
	struct conf_state *conf = &conf_state;

	ASSERT(path != NULL);

	return conf_path_set(conf, path, NULL);
}

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
json_t *conf_get(const char *path)
{
	struct conf_state *conf = &conf_state;

	ASSERT(path != NULL);

	return conf_path_lookup(conf->root, path);
}

/*
 * Return the startup file path.  Config should be initialized prior to calling
 * this function.
 */
const char *conf_startup_file_path(void)
{
	struct conf_state *conf = &conf_state;

	return conf->startup_file ? conf->startup_file : "";
}

/*
 * Return true if the last config file loaded was the factory file.
 * Returns false once a startup config file is successfully saved.
 * Unsaved changes do not affect the return value.
 */
bool conf_factory_loaded(void)
{
	return conf_state.factory_file_loaded;
}

/*
 * Register a config change callback.  This will be called whenever a config
 * object changed.  The change_callback may be invoked at a higher level in the
 * tree than the actual change, so handlers should filter on the paths they
 * need.
 */
void conf_set_change_callback(void (*callback)(const char *, const json_t *))
{
	conf_state.change_handler = callback;
}
