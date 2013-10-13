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
 * @brief		MIPS architecture core definitions.
 */

#ifndef __ARCH_LOADER_H
#define __ARCH_LOADER_H

/**
 * Offset to apply to a physical address to get a virtual address.
 *
 * On MIPS we always run from the virtual address space. KSEG0 and KSEG1 are
 * windows onto the low 256MB of physical memory, the former is cached while
 * the latter is uncached. The loader runs from KSEG0, and for all generic
 * accesses we also want to go through KSEG0 (i.e. we want to use the cache).
 * Platform code which needs uncached access uses KSEG1 explicitly, this
 * definition is only relevant for generic code.
 */
#define LOADER_VIRT_OFFSET	0x80000000

/** Highest physical address accessible to the loader. */
#define LOADER_PHYS_MAX		0x1fffffff

extern void arch_init(void);

#endif /* __ARCH_LOADER_H */
