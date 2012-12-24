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

#include <loaders/kboot.h>

#include <pc/bios.h>
#include <pc/vbe.h>
#include <pc/memory.h>

#include <config.h>
#include <ui.h>

/** Parse video parameters in a KBoot image.
 * @param loader	KBoot loader data structure. */
void kboot_platform_video_init(kboot_loader_t *loader) {
	// TODO
}

/** Perform platform-specific setup for a KBoot kernel.
 * @param loader	KBoot loader data structure. */
void kboot_platform_setup(kboot_loader_t *loader) {
	// TODO: E820, video mode.
}
