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
 * @brief		BCM2835 core definitions.
 */

#ifndef __BCM2835_BCM2835_H
#define __BCM2835_BCM2835_H

/** Physical memory layout definitions. */
#define BCM2835_SDRAM_BASE	0x0		/**< Base address of SDRAM. */
#define BCM2835_SDRAM_SIZE	0x20000000	/**< Maximum SDRAM area size. */
#define BCM2835_PERIPH_BASE	0x20000000	/**< Base address of peripheral area. */
#define BCM2835_PERIPH_SIZE	0x1000000	/**< Size of peripheral area. */

/** Memory mapped peripheral addresses. */
#define BCM2835_GPIO_BASE	(BCM2835_PERIPH_BASE + 0x200000)
#define BCM2835_UART0_BASE	(BCM2835_PERIPH_BASE + 0x201000)

#endif /* __BCM2835_BCM2835_H */
