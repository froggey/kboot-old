/*
 * Copyright (C) 2013 Alex Smith
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
 * @brief		BCM2835 framebuffer driver.
 */

#ifndef __BCM2835_FB_H
#define __BCM2835_FB_H

#include <types.h>

/** Framebuffer information structure. */
typedef struct fb_info {
	uint32_t width;			/**< Width. */
	uint32_t height;		/**< Height. */
	uint32_t pitch;			/**< Number of bytes between each row. */
	uint32_t depth;			/**< Bits per pixel. */
	uint8_t red_size;		/**< Size of red component of each pixel. */
	uint8_t red_pos;		/**< Bit position of the red component of each pixel. */
	uint8_t green_size;		/**< Size of green component of each pixel. */
	uint8_t green_pos;		/**< Bit position of the green component of each pixel. */
	uint8_t blue_size;		/**< Size of blue component of each pixel. */
	uint8_t blue_pos;		/**< Bit position of the blue component of each pixel. */
	uint32_t phys;			/**< Physical address of the framebuffer. */
	uint32_t size;			/**< Size of the framebuffer. */
} fb_info_t;

extern void fb_init(fb_info_t *info);

#endif /* __BCM2835_FB_H */
