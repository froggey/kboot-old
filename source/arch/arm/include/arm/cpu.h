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
 * @brief		ARM CPU definitions.
 */

#ifndef __ARM_CPU_H
#define __ARM_CPU_H

/** Definitions of bits in SCTLR (System Control Register). */
#define ARM_SCTLR_M		(1<<0)	/**< MMU enable. */
#define ARM_SCTLR_A		(1<<1)	/**< Alignment fault checking enable. */
#define ARM_SCTLR_C		(1<<2)	/**< Data and unified caches enable. */
#define ARM_SCTLR_Z		(1<<11)	/**< Branch prediction enable. */
#define ARM_SCTLR_I		(1<<12)	/**< Instruction cache enable. */
#define ARM_SCTLR_V		(1<<13)	/**< Hivecs enable. */

/** Definitions of mode bits in CPSR. */
#define ARM_MODE_USR		0x10	/**< User. */
#define ARM_MODE_FIQ		0x11	/**< FIQ. */
#define ARM_MODE_IRQ		0x12	/**< IRQ. */
#define ARM_MODE_SVC		0x13	/**< Supervisor. */
#define ARM_MODE_ABT		0x17	/**< Abort. */
#define ARM_MODE_UND		0x1B	/**< Undefined. */

#endif /* __ARM_CPU_H */
