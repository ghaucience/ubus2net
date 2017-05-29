/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <stdarg.h>

#include <ayla/nameval.h>
#include <ayla/log.h>

/*
 * Terminal escape codes for color.
 */
#define LOG_DEFAULT		39
#define LOG_RED			31
#define LOG_GREEN		32
#define LOG_YELLOW		33
#define LOG_BLUE		34
#define LOG_MAGENTA		35
#define LOG_CYAN		36
#define LOG_LIGHTGRAY		37
#define LOG_DARKGRAY		90
#define LOG_LIGHTRED		91
#define LOG_LIGHTGREEN		92
#define LOG_LIGHTYELLOW		93
#define LOG_LIGHTBLUE		94
#define LOG_LIGHTMAGENTA	95
#define LOG_LIGHTCYAN		96
#define LOG_WHITE		97

/*
 * Macros to generate terminal escape sequences for color and formatting.
 */
#ifndef LOG_FMT_DISABLE
#define LOG_FMT_ESC(code)	"\e[" #code "m"
#else
#define LOG_FMT_ESC(code)
#endif
#define LOG_FMT(code)		LOG_FMT_ESC(code)
#define LOG_FMT_BOLD(code)	LOG_FMT_ESC(1) LOG_FMT_ESC(code)
#define LOG_FMT_CLEAR		LOG_FMT_ESC(0)

static unsigned ayla_log_options;		/* global log features mask */
static char ayla_log_id_str[LOG_MAX_ID_SIZE];	/* log instance identity */
static enum log_subsystem base_subsystem;	/* global default subsystem */

/* forward declarations */
static void log_console_base(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list args);
static void log_syslog_base(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list args);

/* console logging function */
static void (*ayla_log_console_func)(const char *, enum log_level,
    enum log_subsystem, const char *, va_list) = log_console_base;
/* Syslog logging function */
static void (*ayla_log_syslog_func)(const char *, enum log_level,
    enum log_subsystem, const char *, va_list) = log_syslog_base;


/*
 * Log level and subsystem name tables (defined in header).
 */
DEF_NAMEVAL_TABLE(ayla_log_level_names, AYLA_LOG_LEVELS);
DEF_NAMEVAL_TABLE(ayla_subsystem_names, AYLA_LOG_SUBSYSTEMS);


/*
 * Get the output stream (stdout or stderr) based on the severity.
 */
static FILE *log_get_console_stream(enum log_level level)
{
	if (level >= LOG_AYLA_WARN) {
		return stderr;
	}
	return stdout;
}

/*
 * Return a string literal with the console log message prefix.
 */
static const char *log_get_console_tag(enum log_level level)
{
	if (level <= LOG_AYLA_DEBUG) {
		return "[" LOG_FMT_BOLD(LOG_BLUE) "DBG" LOG_FMT_CLEAR "]";
	}
	if (level <= LOG_AYLA_INFO) {
		return "[" LOG_FMT_BOLD(LOG_GREEN) "INF" LOG_FMT_CLEAR "]";
	}
	if (level <= LOG_AYLA_WARN) {
		return "[" LOG_FMT_BOLD(LOG_YELLOW) "WRN" LOG_FMT_CLEAR "]";
	}
	return "[" LOG_FMT_BOLD(LOG_RED) "ERR" LOG_FMT_CLEAR "]";
}

/*
 * Return the appropriate Syslog severity based on the message's log level.
 */
static int log_get_syslog_severity(enum log_level level)
{
	if (level <= LOG_AYLA_DEBUG) {
		return LOG_DEBUG;
	}
	if (level <= LOG_AYLA_INFO) {
		return LOG_INFO;
	}
	if (level <= LOG_AYLA_WARN) {
		return LOG_WARNING;
	}
	return LOG_ERR;
}

/*
 * If optional subsystem byte is set, convert it to a subsystem value
 * and set the fmt pointer to the beginning of the message.
 * If no subsystem is set, return -1.
 */
static enum log_subsystem log_parse_subsystem(const char **fmt)
{
	unsigned char byte = (*fmt)[0];

	if (byte >= AYLA_SUBSYSTEM_CODE_BASE &&
	    byte < AYLA_SUBSYSTEM_CODE(LOG_SUB_NUM)) {
			++(*fmt);
			return AYLA_SUBSYSTEM_ENUM(byte);
	}
	return -1;
}

/*
 * Write a timestamp string to the supplied buffer in the form:
 * yyyy-mm-ddThh:mm:ss.sss
 * size should be at least 24 bytes to fit the entire timestamp.
 * Returns the number of characters written
 */
size_t log_get_timestamp(char *buf, size_t size)
{
	struct timeval tv;
	struct tm cal;
	size_t len;

	gettimeofday(&tv, NULL);
	gmtime_r(&tv.tv_sec, &cal);

	len = snprintf(buf, size,
	     "%04d-%02d-%02dT%02d:%02d:%02d.%03u",
	      cal.tm_year + 1900,
	      cal.tm_mon + 1,
	      cal.tm_mday,
	      cal.tm_hour,
	      cal.tm_min,
	      cal.tm_sec,
	      (unsigned)(tv.tv_usec / 1000));
	if (len < size) {
		return len;
	}
	return size - 1;
}

/*
 * Lookup a log level by name.
 * Returns -1 if not found.
 */
int log_get_level_val(const char *str)
{
	return lookup_by_name(ayla_log_level_names, str);
}

/*
 * Lookup a log subsystem by name.
 * Returns -1 if not found.
 */
int log_get_subsystem_val(const char *str)
{
	return lookup_by_name(ayla_subsystem_names, str);
}

/*
 * Return a statically allocated string name for the log level.
 * Returns NULL if level is not valid.
 */
const char *log_get_level_name(enum log_level val)
{
	if (val < 0 || val >= LOG_AYLA_NUM_LEVELS) {
		return NULL;
	}
	return ayla_log_level_names[val].name;
}

/*
 * Return a statically allocated string name for the log subsystem.
 * Returns NULL if subsystem is not valid.
 */
const char *log_get_subsystem_name(enum log_subsystem val)
{
	if (val < 0 || val >= LOG_SUB_NUM) {
		return NULL;
	}
	return ayla_subsystem_names[val].name;
}

/*
 * Format and print a log message to the console.  This is the default
 * console logging function.
 *
 * func - the function name or NULL
 * level - log level
 * subsystem - a valid subsystem index, or -1 for no subsystem
 * fmt, args - message format string and argument list
 */
static void log_console_base(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list args)
{
	size_t len = 0;
	char fmt_buf[LOG_MAX_FMT_STR_SIZE];

	/* prepend with optional timestamp */
	if (ayla_log_options & LOG_OPT_TIMESTAMPS) {
		len += log_get_timestamp(fmt_buf, sizeof(fmt_buf));
		fmt_buf[len] = ' '; /* add trailing space */
		++len;
	}

	/* add the level tag */
	len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
	    "%s ", log_get_console_tag(level));

	/* optional log instance name */
	if (*ayla_log_id_str) {
		len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
		    "%s: ", ayla_log_id_str);
		if (len >= sizeof(fmt_buf)) {
			len = 0;
		}
	}

	/* optional function name */
	if ((ayla_log_options & LOG_OPT_FUNC_NAMES) && func) {
		if (*ayla_log_id_str && len > 0) {
			fmt_buf[len - 1] = ':';
		}
		len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
		    "%s() ", func);
		if (len >= sizeof(fmt_buf)) {
			len = 0;
		}
	}

	/* complete format string */
	len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len, " %s\n", fmt);

	if (len >= sizeof(fmt_buf)) {
		/* if fmt_buf is too small, just print the message */
		vfprintf(log_get_console_stream(level), fmt, args);
	} else {
		vfprintf(log_get_console_stream(level), fmt_buf, args);
	}
}

/*
 * Format and log a message to Syslog.  This is the default Syslog
 * logging function.
 *
 * func - the function name or NULL
 * level - log level
 * subsystem - a valid subsystem index, or -1 for no subsystem
 * fmt, args - message format string and argument list
 */
static void log_syslog_base(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list args)
{
	size_t len = 0;
	char fmt_buf[LOG_MAX_FMT_STR_SIZE];

	/* set verbosity and optional subsystem */
	if (subsystem != -1) {
		len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
		    "[%s-%s] ", log_get_level_name(level),
		    log_get_subsystem_name(subsystem));
	} else {
		len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
		    "[%s] ", log_get_level_name(level));
	}
	if (len >= sizeof(fmt_buf)) {
		return;
	}

	/* optional function name */
	if ((ayla_log_options & LOG_OPT_FUNC_NAMES) && func) {
		len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
		    "%s: ", func);
		if (len >= sizeof(fmt_buf)) {
			len = 0;
		}
	}

	/* complete format string */
	len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len, "%s", fmt);

	if (len >= sizeof(fmt_buf)) {
		/* if fmt_buf is too small, just log the message */
		vsyslog(log_get_syslog_severity(level), fmt, args);
	} else {
		vsyslog(log_get_syslog_severity(level), fmt_buf, args);
	}
}

/*
 * Set the name of the logging instance.  Syslog does not store the
 * identity name internally, so it is stored here in case the source
 * string goes out of scope.  Syslog ID is set to default if identity is NULL.
 */
void log_set_identity(const char *identity)
{
	closelog();
	if (identity) {
		snprintf(ayla_log_id_str, sizeof(ayla_log_id_str), "%s",
		    identity);
		openlog(ayla_log_id_str, 0, AYLA_SYSLOG_FACILITY);
	} else {
		ayla_log_id_str[0] = '\0';
		openlog(NULL, 0, AYLA_SYSLOG_FACILITY);
	}
}

/*
 * Initialize logging and set options.
 * If no identity desired, set identity to NULL.
 * If no options required, set options to LOG_OPT_NONE
 */
void log_init(const char *identity, unsigned options)
{
	ayla_log_console_func = log_console_base;
	ayla_log_syslog_func = log_syslog_base;
	ayla_log_options = options;
	base_subsystem = -1;
	log_set_identity(identity);
}

/*
 * Enable one or more log options
 */
void log_set_options(unsigned options)
{
	ayla_log_options |= options;
}

/*
 * Disable one or more log options
 */
void log_clear_options(unsigned options)
{
	ayla_log_options &= ~options;
}

/*
 * Set a default subsystem for the process.
 */
void log_set_subsystem(enum log_subsystem subsystem)
{
	base_subsystem = subsystem;
}

/*
 * Override the default console logging function
 */
void log_set_console_func(void (*func)(const char *,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *fmt, va_list))
{
	ayla_log_console_func = func;
}

/*
 * Override the default syslog logging function
 */
void log_set_syslog_func(void (*func)(const char *,
    enum log_level level, enum log_subsystem,
    const char *fmt, va_list))
{
	ayla_log_syslog_func = func;
}

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
    const char *fmt, ...)
{
	va_list args;
	enum log_subsystem inline_subsystem;

	/* ignore all messages at or below DEBUG level if debug is disabled */
	if (level <= LOG_AYLA_DEBUG && !(ayla_log_options & LOG_OPT_DEBUG)) {
		return;
	}
	inline_subsystem = log_parse_subsystem(&fmt);
	if (inline_subsystem != -1) {
		/* use an inline subsystem, if there is one */
		subsystem = inline_subsystem;
	} else if (subsystem == -1) {
		/* use the default subsystem if no subsystem is set */
		subsystem = base_subsystem;
	}
	/* log to the console */
	if (ayla_log_options & LOG_OPT_CONSOLE_OUT) {
		va_start(args, fmt);
		ayla_log_console_func(func, level, subsystem, fmt, args);
		va_end(args);
	}
	/* log to Syslog */
	if (!(ayla_log_options & LOG_OPT_NO_SYSLOG)) {
		va_start(args, fmt);
		ayla_log_syslog_func(func, level, subsystem, fmt, args);
		va_end(args);
	}
}

/*
 * Hex dump to log message
 */
void log_base_hex(const char *func,
    enum log_level level,
    enum log_subsystem subsystem,
    const char *msg,
    const void *buf, size_t len)
{
	char out[100];
	int tlen;
	int i;
	size_t off = 0;
	const uint8_t *bp = buf;

	if (msg && *msg) {
		log_base_subsystem(func, level, subsystem, "%s (%zu byte%s)",
		    msg, len, len == 1 ? "" : "s");
	} else {
		log_base_subsystem(func, level, subsystem, "%zu byte%s", len,
		    len == 1 ? "" : "s");
	}
	while (off < len) {
		tlen = snprintf(out, sizeof(out) - 2, "%04zX:  ", off);
		for (i = 0; i < 16 && off < len; i++, off++) {
			tlen += snprintf(out + tlen, sizeof(out) - 2 - tlen,
			    "%2.2X %s", bp[off], ((i + 1) % 4) ? "" : " ");
		}
		out[tlen] = '\0';
		log_base_subsystem(func, level, subsystem, "%s", out);
	}
}

/*
 * Return 1 if debug logging is enabled, otherwise 0.
 */
uint8_t log_debug_enabled()
{
	return (ayla_log_options & LOG_OPT_DEBUG) ? 1 : 0;
}
