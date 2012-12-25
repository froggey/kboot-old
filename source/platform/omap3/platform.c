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
 * @brief		OMAP3 platform startup code.
 */

#include <arm/atag.h>

#include <omap3/omap3.h>
#include <omap3/uart.h>

#include <loader.h>
#include <memory.h>
#include <tar.h>

extern void platform_init(atag_t *atags);

/** Main function of the OMAP3 loader.
 * @param atags		ATAG list from U-Boot. */
void platform_init(atag_t *atags) {
	/* Set up the UART for debug output. */
	uart_init();

	/* Initialize the architecture. */
	arch_init(atags);

	/* The boot image is passed to us as an initial ramdisk. */
	ATAG_ITERATE(tag, ATAG_INITRD2) {
		tar_mount((void *)tag->initrd.start, tag->initrd.size);
		break;
	}

	/* Initialize hardware. */
	memory_init();

	/* Call the main function. */
	loader_main();
}
