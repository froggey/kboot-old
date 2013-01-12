/*
 * Copyright (C) 2011-2013 Alex Smith
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
 * @brief		ARM kernel entry code.
 */

#include <arm/asm.h>
#include <kboot.h>

#if CONFIG_KBOOT_LOADER_KBOOT

#define ENTRY_ARGS_TRANSITION_TTBR	0
#define ENTRY_ARGS_VIRT			4
#define ENTRY_ARGS_KERNEL_TTBR		8
#define ENTRY_ARGS_SP			12
#define ENTRY_ARGS_ENTRY		16
#define ENTRY_ARGS_TAGS			20
#define ENTRY_ARGS_TRAMPOLINE		24

/** Enter an ARM KBoot kernel.
 * @param args		Entry arguments structure. */
FUNCTION_START(kboot_arch_enter32)
	/* Ensure that IRQs/FIQs are disabled. */
	cpsid	if

	/* Write the transition L1 table address to TTBR0. */
	ldr	r4, [r0, #ENTRY_ARGS_TRANSITION_TTBR]
	mcr	p15, 0, r4, c2, c0, 0

	/* Ensure that TTBCR is configured to always use TTBR0. */
	mov	r4, #0
	mcr	p15, 0, r4, c2, c0, 2

	/* Enable full access to all domains. */
	mvn	r4, #0
	mcr	p15, 0, r4, c3, c0, 0

	/* Enable the I/D caches (SCTLR.I and SCTLR.C). */
	mrc	p15, 0, r4, c1, c0, 0
	orr	r4, r4, #(1<<12)
	orr	r4, r4, #(1<<2)
	mcr	p15, 0, r4, c1, c0, 0

	/* Invalidate I/D TLB. */
	mcr	p15, 0, r0, c8, c7, 0

	/* Enable the MMU by setting SCTLR.M to 1. */
	orr	r4, r4, #(1<<0)
	mcr	p15, 0, r4, c1, c0, 0
	isb

	/* Calculate the trampoline address. */
	ldr	r4, [r0, #ENTRY_ARGS_VIRT]
	add	r4, r4, #ENTRY_ARGS_TRAMPOLINE
	mov	pc, r4

SYMBOL(kboot_trampoline32)
	/* Write the main L1 table address to TTBR0. */
	ldr	r4, [r0, #ENTRY_ARGS_KERNEL_TTBR]
	mcr	p15, 0, r4, c2, c0, 0
	isb

	/* Set the stack pointer. */
	ldr	sp, [r0, #ENTRY_ARGS_SP]

	/* Clear the stack frame pointer (R11). */
	mov	fp, #0

	/* Call the kernel. */
	ldr	r4, [r0, #ENTRY_ARGS_ENTRY]
	ldr	r1, [r0, #ENTRY_ARGS_TAGS]
	ldr	r0, =KBOOT_MAGIC
	mov	pc, r4
1:	b	1b

SYMBOL(kboot_trampoline32_size)
	.long	. - kboot_trampoline32 + 8
FUNCTION_END(kboot_arch_enter32)

#endif /* CONFIG_KBOOT_LOADER_KBOOT */
