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
 * @brief		PC platform KBoot loader.
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <loaders/kboot.h>

#include <pc/bios.h>
#include <pc/console.h>
#include <pc/vbe.h>
#include <pc/memory.h>

#include <config.h>
#include <memory.h>
#include <ui.h>

/** Default video mode parameters. */
static kboot_itag_video_t default_video_itag = {
	.types = KBOOT_VIDEO_VGA,
	.width = 0,
	.height = 0,
	.bpp = 0,
};

/** Parse the video mode string.
 * @param string	Mode string.
 * @param modep		Where to store VBE mode, if any.
 * @return		Video mode type, or 0 if invalid/not found. */
static uint32_t parse_video_mode(const char *string, vbe_mode_t **modep) {
	uint16_t width = 0, height = 0;
	char *dup, *orig, *tok;
	uint8_t depth = 0;

	if(strcmp(string, "vga") == 0)
		return KBOOT_VIDEO_VGA;

	dup = orig = kstrdup(string);

	if((tok = strsep(&dup, "x")))
		width = strtol(tok, NULL, 0);
	if((tok = strsep(&dup, "x")))
		height = strtol(tok, NULL, 0);
	if((tok = strsep(&dup, "x")))
		depth = strtol(tok, NULL, 0);

	kfree(orig);

	if(width && height) {
		*modep = vbe_mode_find(width, height, depth);
		return (*modep) ? KBOOT_VIDEO_LFB : 0;
	}

	return 0;
}

#if CONFIG_KBOOT_UI

/** Create the UI chooser.
 * @param loader	KBoot loader data structure.
 * @param types		Supported video mode types.
 * @param entry		Environment entry to modify. */
static void create_mode_chooser(kboot_loader_t *loader, uint32_t types, value_t *entry) {
	ui_entry_t *chooser;
	vbe_mode_t *mode;
	value_t value;
	char buf[16];

	value.type = VALUE_TYPE_STRING;
	value.string = buf;

	/* There will only be more than 1 choice if using VBE. */
	if(!(types & KBOOT_VIDEO_LFB) || list_empty(&vbe_modes))
		return;

	chooser = ui_chooser_create("Video Mode", entry);

	if(types & KBOOT_VIDEO_VGA) {
		sprintf(buf, "vga");
		ui_chooser_insert(chooser, "VGA", &value);
	}

	LIST_FOREACH(&vbe_modes, iter) {
		mode = list_entry(iter, vbe_mode_t, header);

		sprintf(buf, "%ux%ux%u", mode->info.x_resolution,
			mode->info.y_resolution,
			mode->info.bits_per_pixel);
		ui_chooser_insert(chooser, NULL, &value);
	}

	ui_list_insert(loader->config, chooser, false);
}

#endif

/** Determine the default video mode.
 * @param tag		Video mode tag.
 * @param modep		Where to store default mode.
 * @return		Type of the mode, or 0 on failure. */
static uint32_t get_default_mode(kboot_itag_video_t *tag, vbe_mode_t **modep) {
	vbe_mode_t *mode;

	if(tag->types & KBOOT_VIDEO_LFB) {
		/* If the kernel specifies a preferred mode, try to find it. */
		mode = (tag->width && tag->height)
			? vbe_mode_find(tag->width, tag->height, tag->bpp)
			: NULL;
		if(!mode)
			mode = default_vbe_mode;

		
		if(mode) {
			*modep = mode;
			return KBOOT_VIDEO_LFB;
		}
	}

	if(tag->types & KBOOT_VIDEO_VGA)
		return KBOOT_VIDEO_VGA;

	return 0;
}

/** Parse video parameters in a KBoot image.
 * @param loader	KBoot loader data structure. */
void kboot_platform_video_init(kboot_loader_t *loader) {
	kboot_itag_video_t *tag;
	vbe_mode_t *mode = NULL;
	value_t *entry, value;
	uint32_t type = 0;
	char buf[16];

	tag = kboot_itag_find(loader, KBOOT_ITAG_VIDEO);
	if(!tag)
		tag = &default_video_itag;

	/* If the kernel doesn't want anything, we don't need to do anything. */
	if(!tag->types) {
		environ_remove(current_environ, "video_mode");
		return;
	}

	/* Check if we already have a valid video mode set in the environment. */
	entry = environ_lookup(current_environ, "video_mode");
	if(entry && entry->type == VALUE_TYPE_STRING)
		type = tag->types & parse_video_mode(entry->string, &mode);

	/* Get the default mode if there wasn't or it is not valid. */
	if(!type)
		type = get_default_mode(tag, &mode);

	/* If there's still nothing, we can't give video information to the
	 * kernel so just remove the environment variable and return. */
	if(!type) {
		environ_remove(current_environ, "video_mode");
		return;
	}

	/* Save the mode in a properly formatted string. */
	switch(type) {
	case KBOOT_VIDEO_LFB:
		sprintf(buf, "%ux%ux%u", mode->info.x_resolution,
			mode->info.y_resolution,
			mode->info.bits_per_pixel);
		break;
	case KBOOT_VIDEO_VGA:
		sprintf(buf, "vga");
		break;
	}

	value.type = VALUE_TYPE_STRING;
	value.string = buf;
	entry = environ_insert(current_environ, "video_mode", &value);

	#if CONFIG_KBOOT_UI
	/* Add a video mode chooser to the UI. */
	create_mode_chooser(loader, tag->types, entry);
	#endif
}

/** Set the video mode.
 * @param loader	KBoot loader data structure. */
static void set_video_mode(kboot_loader_t *loader) {
	vbe_mode_t *mode = NULL;
	kboot_tag_video_t *tag;
	value_t *entry;
	uint32_t type;

	/* In kboot_platform_video_init(), we ensure that the video_mode
	 * environment variable contains a valid mode, so we just need to
	 * grab that and set it. */
	entry = environ_lookup(current_environ, "video_mode");
	if(!entry)
		return;

	type = parse_video_mode(entry->string, &mode);

	/* Create the tag. */
	tag = kboot_allocate_tag(loader, KBOOT_TAG_VIDEO, sizeof(*tag));
	tag->type = type;

	/* Set the mode and save information. */
	switch(type) {
	case KBOOT_VIDEO_VGA:
		tag->vga.cols = 80;
		tag->vga.lines = 25;
		vga_cursor_position(&tag->vga.x, &tag->vga.y);
		tag->vga.mem_phys = VGA_MEM_BASE;
		tag->vga.mem_size = ROUND_UP(tag->vga.cols * tag->vga.lines * 2, PAGE_SIZE);
		tag->vga.mem_virt = kboot_allocate_virtual(loader, tag->vga.mem_phys,
			tag->vga.mem_size);
		break;
	case KBOOT_VIDEO_LFB:
		tag->lfb.flags = (mode->info.memory_model == 4) ? KBOOT_LFB_INDEXED : KBOOT_LFB_RGB;
		tag->lfb.width = mode->info.x_resolution;
		tag->lfb.height = mode->info.y_resolution;
		tag->lfb.bpp = mode->info.bits_per_pixel;
		tag->lfb.pitch = (vbe_info.vbe_version_major >= 3)
			? mode->info.lin_bytes_per_scan_line
			: mode->info.bytes_per_scan_line;

		if(tag->lfb.flags & KBOOT_LFB_RGB) {
			tag->lfb.red_size = mode->info.red_mask_size;
			tag->lfb.red_pos = mode->info.red_field_position;
			tag->lfb.green_size = mode->info.green_mask_size;
			tag->lfb.green_pos = mode->info.green_field_position;
			tag->lfb.blue_size = mode->info.blue_mask_size;
			tag->lfb.blue_pos = mode->info.blue_field_position;
		} else if(tag->lfb.flags & KBOOT_LFB_INDEXED) {
			boot_error("TODO: Indexed video modes");
		}

		/* Map the framebuffer. */
		tag->lfb.fb_phys = mode->info.phys_base_ptr;
		tag->lfb.fb_size = ROUND_UP(tag->lfb.height * tag->lfb.pitch, PAGE_SIZE);
		tag->lfb.fb_virt = kboot_allocate_virtual(loader, tag->lfb.fb_phys,
			tag->lfb.fb_size);

		/* Set the mode. */
		vbe_mode_set(mode);
		break;
	}
}

/** Add E820 memory map tags.
 * @param loader	KBoot loader data structure. */
static void add_e820_tags(kboot_loader_t *loader) {
	kboot_tag_t *tag;
	bios_regs_t regs;

	bios_regs_init(&regs);

	do {
		regs.eax = 0xE820;
		regs.edx = E820_SMAP;
		regs.ecx = 64;
		regs.edi = BIOS_MEM_BASE;
		bios_interrupt(0x15, &regs);

		/* If CF is set, the call was not successful. BIOSes are
		 * allowed to return a non-zero continuation value in EBX and
		 * return an error on next call to indicate that the end of the
		 * list has been reached. */
		if(regs.eflags & X86_FLAGS_CF)
			break;

		/* Create a tag for the entry. */
		tag = kboot_allocate_tag(loader, KBOOT_TAG_E820, sizeof(*tag) + regs.ecx);
		memcpy(&tag[1], (void *)BIOS_MEM_BASE, regs.ecx);
	} while(regs.ebx != 0);
}

/** Perform platform-specific setup for a KBoot kernel.
 * @param loader	KBoot loader data structure. */
void kboot_platform_setup(kboot_loader_t *loader) {
	/* Set the video mode. */
	set_video_mode(loader);

	/* Add a copy of the E820 memory map. */
	add_e820_tags(loader);
}
