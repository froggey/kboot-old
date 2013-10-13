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
 * @brief		MIPS Malta platform core definitions.
 */

#ifndef __PLATFORM_LOADER_H
#define __PLATFORM_LOADER_H

/**
 * Load address.
 *
 * As far as I can tell the low 1MB is pretty much unusable. Most of it is
 * either taken by YAMON or hardware. We need to try and stay out of the way
 * of where a kernel might want to load to, so go for a slightly obscure
 * location: 31MB into physical memory (mapped in KSEG0). Unless someone tries
 * to load a really big kernel image, or load their kernel at a silly location,
 * it seems unlikely that this will conflict.
 */
#define LOADER_LOAD_ADDR		0x81f00000

#ifndef __ASM__

extern void platform_init(int argc, char **argv, char **envp, unsigned memsize);

#endif

#endif /* __PLATFORM_LOADER_H */
