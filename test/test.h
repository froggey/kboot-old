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
 * @brief		Test kernel definitions.
 */

#ifndef __TEST_H
#define __TEST_H

#include <console.h>
#include <elf.h>
#include <kboot.h>
#include <loader.h>

#ifdef __LP64__

#define PHYS_MAP_BASE		0xFFFFFFFF00000000
#define PHYS_MAP_SIZE		0x80000000
#define VIRT_MAP_BASE		0xFFFFFFFF80000000
#define VIRT_MAP_SIZE		0x80000000

typedef Elf64_Shdr elf_shdr_t;
typedef Elf64_Sym  elf_sym_t;
typedef Elf64_Addr elf_addr_t;

#else

#define PHYS_MAP_BASE		0x40000000
#define PHYS_MAP_SIZE		0x80000000
#define VIRT_MAP_BASE		0xC0000000
#define VIRT_MAP_SIZE		0x40000000

typedef Elf32_Shdr elf_shdr_t;
typedef Elf32_Sym  elf_sym_t;
typedef Elf32_Addr elf_addr_t;

#endif

#ifdef CONFIG_PLATFORM_OMAP3
# define PHYS_MAP_OFFSET	0x80000000
#else
# define PHYS_MAP_OFFSET	0x0
#endif

/** Get a virtual address from a physical address. */
#define P2V(phys)		((ptr_t)(phys - PHYS_MAP_OFFSET) + PHYS_MAP_BASE)

extern void fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t rgb);
extern void fb_init(kboot_tag_video_t *tag);

extern void console_init(kboot_tag_t *tags);
extern void log_init(kboot_tag_t *tags);

extern void kmain(uint32_t magic, kboot_tag_t *tags);

#endif /* __TEST_H */
