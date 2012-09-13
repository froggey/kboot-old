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
 * @brief		OMAP3 display subsystem interface.
 *
 * @todo		Proper implementation. This is currently only usable
 *			on the BeagleBoard as it just uses what's set up by
 *			the firmware (except for reconfiguring the framebuffer
 *			address).
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <omap3/dss.h>
#include <omap3/omap3.h>

#include <memory.h>
#include <video.h>

/** Detect available video modes. */
void video_init(void) {
	video_mode_t *mode;
	phys_ptr_t paddr;

	/* Allocate a framebuffer address. */
	phys_memory_alloc(ROUND_UP(1280 * 720 * 2, 0x100000), 0x100000, 0, 0, 0, &paddr);

	/* The BeagleBoard's firmware sets us up in 1280x720, little-endian
	 * RGB16 (5:6:5). */
	mode = kmalloc(sizeof(video_mode_t));
	mode->width = 1280;
	mode->height = 720;
	mode->bpp = 16;
	mode->addr = paddr;
	video_mode_add(mode);

	/* Set the default mode. */
	default_video_mode = mode;
}

/** Switch to the specified video mode.
 * @param mode		Mode to switch to. */
void video_enable(video_mode_t *mode) {
	/* This sets mode to 1024x768, just putting here for future reference.
	 * Don't actually want to do it because changing resolution with real
	 * hardware most likely involves changing timings. */
	//*(uint32_t *)0x4805047c = (767 << 16) | 1023;
	//*(uint32_t *)0x4805048c = (767 << 16) | 1023;

	/* Clear the framebuffer. */
	memset((void *)mode->addr, 0, mode->width * mode->height * (mode->bpp / 8));

	/* Reconfigure the framebuffer base address. */
	*(uint32_t *)(OMAP3_DSS_BASE + DSS_DISPC_GFX_BA0) = mode->addr;
	*(uint32_t *)(OMAP3_DSS_BASE + DSS_DISPC_GFX_BA1) = mode->addr;

	/* Write DISPC_CONTROL. The GOLCD bit must be set to 1 for the previous
	 * changes to take effect. */
	*(uint32_t *)(OMAP3_DSS_BASE + DSS_DISPC_CONTROL) = 0x1836b;
}
