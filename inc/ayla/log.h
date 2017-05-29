/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#ifndef __AYLA_LOG_H__
#define __AYLA_LOG_H__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <ayla/token_table.h>

#define LOG_MAX_ID_SIZE	(64)		/* max chars allowed in ID string */
#define LOG_MAX_FMT_STR_SIZE	(256)	/* reasonable format string limit */
#define LOG_MIN_TIMESTAMP_SIZE	(24)	/* min buffer size for timestamp */

/*
 * Ayla extended log messages use the following facility:
 */
#define AYLA_SYSLOG_FACILITY	LOG_LOCAL0

/*
 * Subsystem may be specified as the leading byte of a log call format
 * string.  AYLA_SUBSYSTEM_CODE_BASE is the starting offset of
 * this subsystem byte.
 */
#define AYLA_SUBSYSTEM_CODE_BASE	0x80
#define AYLA_SUBSYSTEM_CODE(sub)	\
    (unsigned char)(AYLA_SUBSYSTEM_CODE_BASE + (sub & 0xff))
#define AYLA_SUBSYSTEM_ENUM(code)	\
    (enum log_subsystem)((unsigned char)code - AYLA_SUBSYSTEM_CODE_BASE)

/*
 * Ayla custom log level definitions. Up to 32 levels supported.
 * Levels are are defined in order from less severe to more severe.
 * Format: def(<string name>, <enum name>)
 */
#define AYLA_LOG_LEVELS(def)			\
	def(unknown,	LOG_AYLA_UNKNOWN)	\
	def(debug2,	LOG_AYLA_DEBUG2)	\
	def(debug,	LOG_AYLA_DEBUG)		\
	def(metric,	LOG_AYLA_METRIC)	\
	def(info,	LOG_AYLA_INFO)		\
	def(warning,	LOG_AYLA_WARN)		\
	def(error,	LOG_AYLA_ERR)		\
						\
	def(NULL,	LOG_AYLA_NUM_LEVELS)

/*
 * Ayla logging subsystem definitions.
 * Format: def(<string name>, <enum name>)
 */
#define AYLA_LOG_SUBSYSTEMS(def)		\
	/* used for non-Ayla msgs in Syslog */	\
	def(syslog,	LOG_SUB_SYSLOG)		\
	/* shared subsystems */		\
	def(client,	LOG_SUB_CLIENT)		\
	def(conf,	LOG_SUB_CONF)		\
	def(io,		LOG_SUB_IO)		\
	def(dnss,	LOG_SUB_DNSS)		\
	def(mod,	LOG_SUB_MOD)		\
	def(server,	LOG_SUB_SERVER)		\
	def(ssl,	LOG_SUB_SSL)		\
	def(wifi,	LOG_SUB_WIFI)		\
	/* Linux-specific subsystems */		\
	def(gateway,	LOG_SUB_GATEWAY)	\
	def(proxy,	LOG_SUB_PROXY)		\
	def(cli,	LOG_SUB_CLI)		\
	def(app,	LOG_SUB_APP)		\
	def(logger,	LOG_SUB_LOGGER)		\
	def(ota,	LOG_SUB_OTA)		\
						\
	def(NULL,	LOG_SUB_NUM)

/*
 * Ayla log message subsystem prefixes.
 * These MUST match the definitions in the AYLA_LOG_SUBSYSTEMS list.
 */
/* #define LOG_SYSLOG	"\x80"	start at AYLA_SUBSYSTEM_CODE_BASE */
#define LOG_CLIENT	"\x81"
#define LOG_CONF	"\x82"
#define LOG_IO		"\x83"
#define LOG_DNSS	"\x84"
#define LOG_MOD		"\x85"
#define LOG_SERVER	"\x86"
#define LOG_SSL		"\x87"
#define LOG_WIFI	"\x88"
#define LOG_GATEWAY	"\x89"
#define LOG_PROXY	"\x8a"
#define LOG_CLI		"\x8b"
#define LOG_APP		"\x8c"
#define LOG_LOGGER	"\x8d"
#define LOG_OTA		"\x8e"

/*
 * Define Ayla log level and subsystem enums
 */
DEF_ENUM(log_level, AYLA_LOG_LEVELS);
DEF_ENUM(log_subsystem, AYLA_LOG_SUBSYSTEMS);

/*
 * Logging option flags	(One-hot encoded)
 */
enum ayla_log_options {
	LOG_OPT_NONE		= 0x0000,
	LOG_OPT_CONSOLE_OUT	= 0x0001,
	LOG_OPT_TIMESTAMPS	= 0x0002,
	LOG_OPT_FUNC_NAMES	= 0x0004,
	LOG_OPT_NO_SYSLOG	= 0x0008,
	LOG_OPT_DEBUG		= 0x0010
};

/*
 * Format and log a message.  Parses an optional subsystem code
 * embedded in the first byte of the format string, or uses the
 * subsystem parameter.
 * Intended to be used by log macros.
 *
 * func - the function name or NULL
 * level - log severity
 * subsystem - a valid subsystem index, or -1 for no subsystem
 * fmt... - format string and optional arguments
 */
/*
 * The same as log_base(), but the subsystem can specified as a parameter.
 *
 * func - the function name or NULL
 * level - log severity
 * subsystem - a valid subsystem index, or -1 for no subsystem
 * fmt... - message format string and argument list
 */
void log_base_subsystem(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, ...) __attribute__  ((format (printf, 4, 5)));

/*
 * Hex dump to log message.
 *
 * msg - a tag describing the data (could be the func name)
 * level - log severity
 * subsystem - a valid subsystem index, or -1 for no subsystem
 * buf - data to hex dump
 * len - number of bytes to dump
 */
void log_base_hex(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *msg,
    const void *buf, size_t len);

/*
 * Log interface macros
 */
#define log_base(func, level, ...)	\
	log_base_subsystem(func,	level,	-1,	__VA_ARGS__)
#define log_info(...)	\
	log_base(__func__,	LOG_AYLA_INFO,		__VA_ARGS__)
#define log_metric(...)	\
	log_base(__func__,	LOG_AYLA_METRIC,	__VA_ARGS__)
#define log_debug(...)	\
	log_base(__func__,	LOG_AYLA_DEBUG,		__VA_ARGS__)
#ifdef BUILD_RELEASE
#define log_debug2(...)
#else
#define log_debug2(...)	\
	log_base(__func__,	LOG_AYLA_DEBUG2,	__VA_ARGS__)
#endif
#define log_warn(...)	\
	log_base(__func__,	LOG_AYLA_WARN,		__VA_ARGS__)
#define log_err(...)	\
	log_base(__func__,	LOG_AYLA_ERR,		__VA_ARGS__)

#define log_debug_hex(msg, buf, len)	\
	log_base_hex(__func__,	LOG_AYLA_DEBUG,		-1, msg, buf, len)
#ifdef BUILD_RELEASE
#define log_debug2_hex(msg, buf, len)
#else
#define log_debug2_hex(msg, buf, len)	\
	log_base_hex(__func__,	LOG_AYLA_DEBUG2,	-1, msg, buf, len)
#endif


/*
 * Initialize logging and set options.
 * If no identity desired, set identity to NULL.
 * If no options required, set options to LOG_OPT_NONE
 */
void log_init(const char *identity, unsigned options);

/*
 * Set the name of the logging instance.
 */
void log_set_identity(const char *identity);

/*
 * Enable one or more log options
 */
void log_set_options(unsigned options);

/*
 * Disable one or more log options
 */
void log_clear_options(unsigned options);

/*
 * Set the subsystem to allow more granular filtering of log entries.
 * This function sets a default subsystem for the entire process.  Individual
 * log calls may override this default by using subsystem code at the beginning
 * of the message:
 * Example: log_info(LOG_MYSUBSYSTEM "my log message");
 */
void log_set_subsystem(enum log_subsystem subsystem);

/*
 * Override the default console logging function
 */
void log_set_console_func(void (*func)(const char *,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list));

/*
 * Override the default syslog logging function
 */
void log_set_syslog_func(void (*func)(const char *,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list));

/*
 * Write a timestamp string to the supplied buffer in the form:
 * yyyy-mm-ddThh:mm:ss.sss
 * size should be at least LOG_MIN_TIMESTAMP_SIZE bytes to fit the entire
 * timestamp.
 * Returns the number of characters written
 */
size_t log_get_timestamp(char *buf, size_t size);

/*
 * Lookup a log level by name.
 * Returns -1 if not found.
 */
int log_get_level_val(const char *str);

/*
 * Lookup a log subsystem by name.
 * Returns -1 if not found.
 */
int log_get_subsystem_val(const char *str);

/*
 * Return a statically allocated string name for the log level.
 * Returns NULL if level is not valid.
 */
const char *log_get_level_name(enum log_level val);

/*
 * Return a statically allocated string name for the log subsystem.
 * Returns NULL if subsystem is not valid.
 */
const char *log_get_subsystem_name(enum log_subsystem val);

/*
 * Return 1 if debug logging is enabled, otherwise 0.
 */
uint8_t log_debug_enabled();

#endif /*  __AYLA_LOG_H__ */
