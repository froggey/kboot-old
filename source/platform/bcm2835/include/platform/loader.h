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
 * @brief		BCM2835 platform core definitions.
 */

#ifndef __PLATFORM_LOADER_H
#define __PLATFORM_LOADER_H

/**
 * Load address.
 *
 * This is where start.elf (running on the VideoCore GPU) will load us to, or
 * where U-Boot should load us if we're booting via that. If config.txt sets
 * disable_commandline_tags we'll get loaded to 0x0 instead, but we require
 * tags so we don't need to support that case.
 *
 * Older firmwares (before May 2012) loaded to 0x0 and required a 32k blob to
 * be prepended to do some setup stuff and jump to 0x8000, this is no longer
 * required - https://github.com/raspberrypi/linux/issues/16
 */
#define LOADER_LOAD_ADDR		0x8000

#ifndef __ASM__

#define platform_early_init()	

#endif /* __ASM__ */
#endif /* __PLATFORM_LOADER_H */
