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

/** Main function for the Kiwi bootloader. */
void loader_main(void) {
	/* Detect hardware details. */
	memory_init();
#if CONFIG_KBOOT_HAVE_DISK
	disk_init();
#endif
	/* We must have a filesystem to boot from. */
	if(!boot_device || !boot_device->fs)
		boot_error("Could not find boot filesystem");

	/* Load the configuration file. */
	config_init();

	/* Display the menu interface if enabled. If not, the root environment
	 * should have the OS loaded. */
#if CONFIG_KBOOT_UI
	current_environ = menu_display();
#endif

	/* Load the operating system. */
	if(!current_environ->device) {
		boot_error("Specified boot device not found");
	} else if(!current_environ->loader) {
		boot_error("No operating system loaded");
	}

	current_environ->loader->load();
}
