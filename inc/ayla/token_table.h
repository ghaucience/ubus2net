/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */
#ifndef __AYLA_TOKEN_TABLE_H__
#define __AYLA_TOKEN_TABLE_H__

/*
 * Convenient macros to generate enums, string tables, and name_val tables
 * from a single token definition list.
 *
 * Item definitions are in the form:
 *   <template>(<string-name>, <enum-name>)
 * For example:
 *   #define SAMPLE_ITEMS(def)		\
 *     def(first,	FIRST_ITEM)	\
 *     def(second,	SECOND_ITEM)	\
 *     def(last,	LAST_ITEM)
 */

/* create enum entry */
#define DEF_ENUM_ENTRY(str_name, enum_name)	enum_name,
/* create name table entry */
#define DEF_NAME_ENTRY(str_name, enum_name)	#str_name,
/* create name_val table entry */
#define DEF_NAMEVAL_ENTRY(str_name, enum_name)	{ #str_name, enum_name },

/* define enum */
#define DEF_ENUM(enum_type, item_defs) \
	enum enum_type { item_defs(DEF_ENUM_ENTRY) }
/* define string table for string to enum lookups*/
#define DEF_NAME_TABLE(table_name, item_defs) \
	const char * const table_name[] = { item_defs(DEF_NAME_ENTRY) }
/* define string table for string to enum lookups*/
#define DEF_NAMEVAL_TABLE(table_name, item_defs) \
	const struct name_val table_name[] = { \
		item_defs(DEF_NAMEVAL_ENTRY) \
		{ NULL, -1 } \
	}

#endif /* __AYLA_TOKEN_TABLE_H__ */
