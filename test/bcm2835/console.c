/*
 * Copyright (C) 2011 Alex Smith
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
 * @brief		Test kernel console functions.
 */

#include <arch/page.h>

#include <lib/utility.h>

#include <bcm2835/bcm2835.h>
#include <bcm2835/gpio.h>
#include <bcm2835/uart.h>

#include <pl011/pl011.h>

#include "../test.h"

KBOOT_MAPPING(0xC1000000, BCM2835_UART0_BASE, PAGE_SIZE);
KBOOT_MAPPING(0xC1001000, BCM2835_GPIO_BASE, PAGE_SIZE);

/** Initialize the console.
 * @param tags		Tag list. */
void console_init(kboot_tag_t *tags) {
	kboot_tag_video_t *video;

	/* Disable the OK LED (loader sets it on, so we can see if we reach here). */
	volatile uint32_t *gpio_mapping = (volatile uint32_t *)0xC1001000;
	gpio_mapping[GPIO_REG_SET0] = 1 << 16;

	pl011_init(0xC1000000, UART0_CLOCK);

	while(tags->type != KBOOT_TAG_NONE) {
		if(tags->type == KBOOT_TAG_VIDEO) {
			video = (kboot_tag_video_t *)tags;

			switch(video->type) {
			case KBOOT_VIDEO_LFB:
				fb_init(video);
				break;
			}

			break;
		}

		tags = (kboot_tag_t *)ROUND_UP((ptr_t)tags + tags->size, 8);
	}
}
