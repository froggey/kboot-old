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
 * @brief		BCM2835 platform KBoot loader.
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <loaders/kboot.h>

#include <bcm2835/fb.h>

#include <config.h>
#include <memory.h>
#include <ui.h>

/** Parse video parameters in a KBoot image.
 * @param loader	KBoot loader data structure. */
void kboot_platform_video_init(kboot_loader_t *loader) {
	/* This function doesn't actually do anything, we only support using
	 * the mode that's already set by the firmware so we don't add any
	 * configuration variables. */
}

/** Set the video mode.
 * @param loader	KBoot loader data structure. */
static void set_video_mode(kboot_loader_t *loader) {
	kboot_itag_video_t *itag;
	kboot_tag_video_t *tag;
	fb_info_t info;

	itag = kboot_itag_find(loader, KBOOT_ITAG_VIDEO);
	if(!itag || !(itag->types & KBOOT_VIDEO_LFB))
		return;

	/* Initialize the framebuffer. */
	fb_init(&info);

	/* Create the tag. */
	tag = kboot_allocate_tag(loader, KBOOT_TAG_VIDEO, sizeof(*tag));
	tag->type = KBOOT_VIDEO_LFB;
	tag->lfb.flags = KBOOT_LFB_RGB;
	tag->lfb.width = info.width;
	tag->lfb.height = info.height;
	tag->lfb.bpp = info.depth;
	tag->lfb.pitch = info.pitch;
	tag->lfb.red_size = info.red_size;
	tag->lfb.red_pos = info.red_pos;
	tag->lfb.green_size = info.green_size;
	tag->lfb.green_pos = info.green_pos;
	tag->lfb.blue_size = info.blue_size;
	tag->lfb.blue_pos = info.blue_pos;

	/* Map the framebuffer. */
	tag->lfb.fb_phys = info.phys;
	tag->lfb.fb_size = ROUND_UP(info.size, PAGE_SIZE);
	tag->lfb.fb_virt = kboot_allocate_virtual(loader, tag->lfb.fb_phys,
		tag->lfb.fb_size);
}

/** Perform platform-specific setup for a KBoot kernel.
 * @param loader	KBoot loader data structure. */
void kboot_platform_setup(kboot_loader_t *loader) {
	/* Set the video mode. */
	set_video_mode(loader);
}
