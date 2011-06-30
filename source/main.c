/*
 * Copyright (C) 2010-2011 Alex Smith
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
 * @brief		KBoot main function.
 */

#include <lib/string.h>

#include <assert.h>
#include <config.h>
#include <console.h>
#include <fs.h>
#include <loader.h>
#include <memory.h>
#include <menu.h>
#include <system.h>
#include <video.h>

extern char __bss_start[], __bss_end[];
extern void loader_main(void);

/** Get the loader type from an environment.
 * @param env		Environment to get from.
 * @return		Pointer to loader type. */
loader_type_t *loader_type_get(environ_t *env) {
	value_t *value;

	value = environ_lookup(env, "loader_type");
	if(!value || value->type != VALUE_TYPE_POINTER) {
		boot_error("No operating system loaded");
	}

	return value->pointer;
}

/** Set the loader type in an environment.
 * @param env		Environment to set in.
 * @param type		Type to set. */
void loader_type_set(environ_t *env, loader_type_t *type) {
	value_t value;

	value.type = VALUE_TYPE_POINTER;
	value.pointer = type;
	environ_insert(env, "loader_type", &value);
}

/** Get the loader data from an environment.
 * @param env		Environment to get from.
 * @return		Pointer to loader data. */
void *loader_data_get(environ_t *env) {
	value_t *value;

	value = environ_lookup(env, "loader_data");
	assert(value && value->type == VALUE_TYPE_POINTER);
	return value->pointer;
}

/** Set the loader data in an environment.
 * @param env		Environment to set in.
 * @param data		Loader data pointer. */
void loader_data_set(environ_t *env, void *data) {
	value_t value;

	value.type = VALUE_TYPE_POINTER;
	value.pointer = data;
	environ_insert(env, "loader_data", &value);
}

/** Main function for the Kiwi bootloader. */
void loader_main(void) {
	loader_type_t *type;
	environ_t *env;
	value_t *value;
	device_t *device;

	/* Initialise the console. */
	console_init();

	/* Perform early architecture/platform initialisation. */
	arch_early_init();
	platform_early_init();
	internal_error("Meow");
	/* Detect hardware details. */
	memory_init();
#if CONFIG_KBOOT_HAVE_DISK
	disk_init();
#endif
	/* We must have a filesystem to boot from. */
	if(!current_device || !current_device->fs) {
		boot_error("Could not find boot filesystem");
	}

#if CONFIG_KBOOT_HAVE_VIDEO
	video_init();
#endif

	/* Load the configuration file. */
	config_init();

#if CONFIG_KBOOT_UI
	/* Display the menu interface. */
	env = menu_display();
#else
	env = root_environ;
#endif

	/* Set the current filesystem. */
	if((value = environ_lookup(env, "device")) && value->type == VALUE_TYPE_STRING) {
		if(!(device = device_lookup(value->string))) {
			boot_error("Could not find device %s", value->string);
		}
		current_device = device;
	}

	/* Load the operating system. */
	type = loader_type_get(env);
	type->load(env);
}
