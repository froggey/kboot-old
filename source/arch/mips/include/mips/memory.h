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
 * @brief		MIPS address space definitions.
 */

#ifndef __MIPS_MEMORY_H
#define __MIPS_MEMORY_H

/** Memory segment definitions. */
#define KUSEG		0x00000000	/**< KUSEG: 2GB, mapped, cached. */
#define KSEG0		0x80000000	/**< KSEG0: 512MB, unmapped, cached. */
#define KSEG1		0xa0000000	/**< KSEG1: 512MB, unmapped, uncached. */
#define KSEG2		0xc0000000	/**< KSEG2: 512MB, mapped, cached. */
#define KSEG3		0xe0000000	/**< KSEG3: 512MB, mapped, cached. */

/** Convert a KSEG0/KSEG1 address to a physical address. */
#define KPHYSADDR(a)	((a) & 0x1fffffff)

/** Convert a physical address to a KSEG0 address. */
#define KSEG0ADDR(a)	(KPHYSADDR(a) | KSEG0)

/** Convert a physical address to a KSEG1 address. */
#define KSEG1ADDR(a)	(KPHYSADDR(a) | KSEG1)

#endif /* __MIPS_MEMORY_H */
