/*
 * Copyright (C) 2010-2012 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief		Configuration system.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <lib/list.h>

#include <fs.h>
#include <system.h>

struct command;
struct device;
struct loader_type;
struct value;

/** Structure containing an environment. */
typedef struct environ {
	struct environ *parent;		/**< Parent environment. */

	/** Values set in the environment by the user. */
	list_t entries;

	/** Per-environment data used internally. */
	struct device *device;		/**< Current device. */
	struct loader_type *loader;	/**< Operating system loader type. */
	void *data;			/**< Data used by the loader. */
} environ_t;

/** Structure containing a list of commands. */
typedef list_t command_list_t;

/** Structure containing a list of values. */
typedef struct value_list {
	struct value *values;		/**< Array of values. */
	size_t count;			/**< Number of arguments. */
} value_list_t;

/** Structure containing a value used in the configuration.  */
typedef struct value {
	/** Type of the value. */
	enum {
		/** Types that can be set from the configuration file. */
		VALUE_TYPE_INTEGER,		/**< Integer. */
		VALUE_TYPE_BOOLEAN,		/**< Boolean. */
		VALUE_TYPE_STRING,		/**< String. */
		VALUE_TYPE_LIST,		/**< List. */
		VALUE_TYPE_COMMAND_LIST,	/**< Command list. */

		/** Types used internally. */
		VALUE_TYPE_POINTER,		/**< Pointer. */
	} type;

	/** Actual value. */
	union {
		uint64_t integer;		/**< Integer. */
		bool boolean;			/**< Boolean. */
		char *string;			/**< String. */
		value_list_t *list;		/**< List. */
		command_list_t *cmds;		/**< Command list. */
		void *pointer;			/**< Pointer. */
	};
} value_t;

/** Structure describing a command that can be used in a command list. */
typedef struct command {
	const char *name;		/**< Name of the command. */

	/** Execute the command.
	 * @param args		List of arguments.
	 * @return		Whether the command completed successfully. */
	bool (*func)(value_list_t *args);
} command_t;

/** Define a command, to be automatically added to the command list. */
#define BUILTIN_COMMAND(name, func) \
	static command_t __command_##func __used = { \
		name, \
		func \
	}; \
	DEFINE_BUILTIN(BUILTIN_TYPE_COMMAND, __command_##func)

extern char *config_file_override;
extern environ_t *root_environ;
extern environ_t *current_environ;

extern void value_copy(value_t *source, value_t *dest);
extern void value_destroy(value_t *value);

extern bool command_list_exec(command_list_t *list, environ_t **envp);

extern void value_list_insert(value_list_t *list, value_t *value);

extern environ_t *environ_create(environ_t *parent);
extern value_t *environ_lookup(environ_t *env, const char *name);
extern void environ_insert(environ_t *env, const char *name, value_t *value);
extern void environ_destroy(environ_t *env);

extern void config_init(void);

#endif /* __CONFIG_H */
