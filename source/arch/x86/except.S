/*
 * Copyright (C) 2010-2011 Alex Smith
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
 * @brief		x86 interrupt service routines.
 */

#include <platform/loader.h>

#include <x86/asm.h>

.section ".text", "ax", @progbits

/** Push registers onto the stack. */
.macro PUSH_REGS
	pusha

	push	%ds
	andl	$0xFFFF, (%esp)
	push	%es
	andl	$0xFFFF, (%esp)
	push	%fs
	andl	$0xFFFF, (%esp)
	push	%gs
	andl	$0xFFFF, (%esp)
.endm

/**
 * Macro to define an ISR.
 *
 * Defines an ISR handler which pushes the interrupt number and a fake error
 * code to the stack. Aligned to 16 bytes because the IDT initialisation
 * code wants each handler to be 16 bytes in size.
 *
 * @param nr		Interrupt vector number.
 */
.macro ISR nr
.align 16
	push	$0
	push	$\nr
	jmp	isr_common
.endm

/**
 * Macro to define an ISR (with error code).
 *
 * Defines an ISR handler which pushes the interrupt numberto the stack. For
 * interrupts defined using this macro, the CPU should have pushed an error
 * code to the stack. Aligned to 16 bytes because the IDT initialisation
 * code wants each handler to be 16 bytes in size.
 *
 * @param nr		Interrupt vector number.
 */
.macro ISR_E nr
.align 16
	push	$\nr
	jmp	isr_common
.endm

/** Array of ISR handlers, each 16 bytes long. */
.align 16
FUNCTION_START(isr_array)
	/* Define the exceptions (0-19) and the reserved interrupts (20-31). */
	ISR	0
	ISR	1
	ISR	2
	ISR	3
	ISR	4
	ISR	5
	ISR	6
	ISR	7
	ISR_E	8
	ISR	9
	ISR_E	10
	ISR_E	11
	ISR_E	12
	ISR_E	13
	ISR_E	14
	ISR	15
	ISR	16
	ISR_E	17
	ISR	18
	ISR	19
	ISR	20
	ISR	21
	ISR	22
	ISR	23
	ISR	24
	ISR	25
	ISR	26
	ISR	27
	ISR	28
	ISR	29
	ISR	30
	ISR	31
FUNCTION_END(isr_array)

/** Common ISR handling code. */
PRIVATE_FUNCTION_START(isr_common)
	/* Create the interrupt frame structure on the stack. */
	PUSH_REGS

	/* Set sane segment descriptors. */
	mov	$SEGMENT_DS, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs

	/* Clear direction flag. */
	cld

	/* Pass a pointer to the interrupt stack frame. */
	push	%esp
	call	interrupt_handler
FUNCTION_END(isr_common)
