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
 *
 * Reference:
 *  - Mailbox Property Interface - Raspberry Pi Firmware Wiki
 *    https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

#include <arch/page.h>

#include <bcm2835/bcm2835.h>
#include <bcm2835/fb.h>
#include <bcm2835/mbox.h>

#include <loader.h>

/** Message to get the current resolution. */
typedef struct msg_get_size {
	prop_message_header_t header;
	prop_get_size_t tag;
	prop_message_footer_t footer;
} __aligned(16) msg_get_size_t;

/** Message to configure the screen. */
typedef struct msg_setup {
	prop_message_header_t header;
	prop_set_size_t set_phys;
	prop_set_size_t set_virt;
	prop_set_depth_t set_depth;
	prop_set_pixel_order_t set_order;
	prop_set_alpha_mode_t set_alpha;
	prop_set_offset_t set_offset;
	prop_allocate_buffer_t allocate;
	prop_message_footer_t footer;
} __aligned(16) msg_setup_t;

/** Message to get the pitch. */
typedef struct msg_get_pitch {
	prop_message_header_t header;
	prop_get_pitch_t tag;
	prop_message_footer_t footer;
} __aligned(16) msg_get_pitch_t;

/** Initialize the framebuffer.
 * @param info		Where to store framebuffer information. */
void fb_init(fb_info_t *info) {
	msg_get_size_t get_size;
	msg_setup_t setup;
	msg_get_pitch_t get_pitch;

	/* Query the current display size. */
	PROP_MESSAGE_INIT(get_size);
	PROP_TAG_INIT(get_size.tag, PROP_TAG_GET_PHYSICAL_SIZE);
	if(!mbox_prop_request(&get_size))
		internal_error("Failed to get current display size");

	dprintf("fb: current display size is %ux%u\n", get_size.tag.resp.width,
		get_size.tag.resp.height);

	info->width = get_size.tag.resp.width;
	info->height = get_size.tag.resp.height;

	/* QEMU Raspberry Pi emulation doesn't return a screen size, try to
	 * set one if we get 0 back. */
	if(info->width == 0 || info->height == 0) {
		info->width = 1024;
		info->height = 768;
	}

	/* Set the depth to 16. TODO: Should we get the current? */
	info->depth = 16;
	info->red_pos = 11;
	info->red_size = 5;
	info->green_pos = 5;
	info->green_size = 6;
	info->blue_pos = 0;
	info->blue_size = 5;

	/* Set up the framebuffer. */
	PROP_MESSAGE_INIT(setup);
	PROP_TAG_INIT(setup.set_phys, PROP_TAG_SET_PHYSICAL_SIZE);
	setup.set_phys.req.width = info->width;
	setup.set_phys.req.height = info->height;
	PROP_TAG_INIT(setup.set_virt, PROP_TAG_SET_VIRTUAL_SIZE);
	setup.set_virt.req.width = info->width;
	setup.set_virt.req.height = info->height;
	PROP_TAG_INIT(setup.set_depth, PROP_TAG_SET_DEPTH);
	setup.set_depth.req.depth = info->depth;
	PROP_TAG_INIT(setup.set_order, PROP_TAG_SET_PIXEL_ORDER);
	setup.set_order.req.state = 0x2;	/* RGB */
	PROP_TAG_INIT(setup.set_alpha, PROP_TAG_SET_ALPHA_MODE);
	setup.set_alpha.req.state = 0x2;	/* Ignored */
	PROP_TAG_INIT(setup.set_offset, PROP_TAG_SET_VIRTUAL_OFFSET);
	setup.set_offset.req.x = 0;
	setup.set_offset.req.y = 0;
	PROP_TAG_INIT(setup.allocate, PROP_TAG_ALLOCATE_BUFFER);
	setup.allocate.req.alignment = PAGE_SIZE;
	if(!mbox_prop_request(&setup))
		internal_error("Failed to set framebuffer configuration");

	if(setup.set_phys.resp.width != info->width || setup.set_phys.resp.height != info->height) {
		internal_error("Failed to set physical display size");
	} else if(setup.set_virt.resp.width != info->width || setup.set_virt.resp.height != info->height) {
		internal_error("Failed to set virtual display size");
	} else if(setup.set_depth.resp.depth != info->depth) {
		internal_error("Failed to set depth");
	} else if(setup.allocate.resp.address % PAGE_SIZE || !setup.allocate.resp.size) {
		internal_error("Failed to set framebuffer (0x%lx, 0x%lx)\n",
			setup.allocate.resp.address, setup.allocate.resp.size);
	}

	info->phys = setup.allocate.resp.address;
	info->size = setup.allocate.resp.size;

	/* Get the pitch. */
	PROP_MESSAGE_INIT(get_pitch);
	PROP_TAG_INIT(get_pitch.tag, PROP_TAG_GET_PITCH);
	if(!mbox_prop_request(&get_pitch))
		internal_error("Failed to get pitch");

	info->pitch = get_pitch.tag.resp.pitch;

	dprintf("fb: set mode %ux%ux%u (framebuffer: 0x%lx, size: 0x%lx, pitch: %u)\n",
		info->width, info->height, info->depth, info->phys, info->size,
		info->pitch);
}
