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
 * @brief		BCM2835 platform startup code.
 */

#include <arm/atag.h>

#include <lib/utility.h>

#include <bcm2835/bcm2835.h>
#include <bcm2835/gpio.h>
#include <bcm2835/uart.h>

#include <loader.h>
#include <memory.h>
#include <tar.h>

/** Main function of the BCM2835 loader.
 * @param atags		ATAG list from the firmware/U-Boot. */
void platform_init(atag_t *atags) {
	/* Light up the OK LED (LED will be lit when pin is clear). */
	gpio_select_function(16, GPIO_FUNC_OUTPUT);
	gpio_clear_pin(16);

	/* Set up the UART for debug output. */
	uart_init();

	dprintf("loader: loaded, ATAGs at %p\n", atags);

	/* Initialize the architecture. */
	arch_init(atags);

	/* The boot image is passed to us as an initial ramdisk. */
	ATAG_ITERATE(tag, ATAG_INITRD2) {
		tar_mount((void *)tag->initrd.start, tag->initrd.size);
		break;
	}

	/* Architecture code adds memory ranges specified by the ATAG list.
	 * Additionally mark the region between the start of SDRAM and our load
	 * address as internal, as the firmware puts things like the ATAG list
	 * here. */
	phys_memory_add(BCM2835_SDRAM_BASE,
		ROUND_DOWN(V2P((ptr_t)__start), PAGE_SIZE) - BCM2835_SDRAM_BASE,
		PHYS_MEMORY_INTERNAL);

	/* Initialize the memory manager. */
	memory_init();

	/* Call the main function. */
	loader_main();
}
