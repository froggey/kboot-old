/*
 * Copyright (C) 2012 Alex Smith
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
 * @brief		Linux kernel loader.
 *
 * This file implements the 'linux' command for loading a Linux kernel. The
 * actual loading work is deferred to the architecture, as each architecture
 * has its own boot protocol. This just implements the actual configuration
 * command and the settings interface.
 *
 * The 'linux' command is used as follows:
 *
 *   linux "<kernel path>" ["<initrd path>"]
 *
 * This loads the kernel and an optional initrd. The kernel command line is
 * set through the cmdline environment variable.
 */

#include <lib/string.h>

#include <assert.h>
#include <fs.h>
#include <loader.h>
#include <memory.h>
#include <ui.h>

/** Structure containing Linux kernel loading arguments. */
typedef struct linux_data {
	const char *kernel;		/**< Kernel image path. */
	const char *initrd;		/**< Initrd path. */
#if CONFIG_KBOOT_UI
	ui_window_t *config;		/**< Configuration window. */
#endif
} linux_data_t;

extern __noreturn void linux_arch_load(file_handle_t *kernel, file_handle_t *initrd,
	const char *cmdline);

/** Load the operating system. */
static __noreturn void linux_loader_load(void) {
	linux_data_t *data = current_environ->data;
	file_handle_t *kernel, *initrd = NULL;
	value_t *cmdline;

	kernel = file_open(data->kernel, NULL);
	if(!kernel)
		boot_error("Failed to open kernel image");

	if(data->initrd) {
		initrd = file_open(data->initrd, NULL);
		if(!initrd)
			boot_error("Failed to open initrd");
	}

	cmdline = environ_lookup(current_environ, "cmdline");
	assert(cmdline->type == VALUE_TYPE_STRING);

	linux_arch_load(kernel, initrd, cmdline->string);
}

#if CONFIG_KBOOT_UI
/** Return a window for configuring the OS.
 * @return		Pointer to configuration window. */
static ui_window_t *linux_loader_configure(void) {
	linux_data_t *data = current_environ->data;
	return data->config;
}
#endif

/** Linux loader type. */
static loader_type_t linux_loader_type = {
	.load = linux_loader_load,
#if CONFIG_KBOOT_UI
	.configure = linux_loader_configure,
#endif
};

/** Load a Linux kernel and initrd.
 * @param args		Command arguments.
 * @return		Whether completed successfully. */
static bool config_cmd_linux(value_list_t *args) {
	value_t *entry, value;
	linux_data_t *data;

	if((args->count != 1 && args->count != 2)
		|| args->values[0].type != VALUE_TYPE_STRING
		|| (args->count == 2 && args->values[1].type != VALUE_TYPE_STRING))
	{
		dprintf("config: linux: invalid arguments\n");
		return false;
	}

	data = kmalloc(sizeof(*data));
	data->kernel = kstrdup(args->values[0].string);
	data->initrd = (args->count == 2) ? kstrdup(args->values[1].string) : NULL;

	/* Add the command line environment variable. */
	entry = environ_lookup(current_environ, "cmdline");
	if(!entry || entry->type != VALUE_TYPE_STRING) {
		value_init(&value, VALUE_TYPE_STRING);
		environ_insert(current_environ, "cmdline", &value);
	}

#if CONFIG_KBOOT_UI
	/* Create the configuration UI. */
	data->config = ui_list_create("Kernel Options", true);
	ui_list_insert(data->config,
		ui_entry_create("Command Line", entry),
		false);
#endif

	current_environ->loader = &linux_loader_type;
	current_environ->data = data;
	return true;
}

BUILTIN_COMMAND("linux", config_cmd_linux);
