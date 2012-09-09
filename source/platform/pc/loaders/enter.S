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
 * @brief		PC kernel entry functions.
 */

#include <platform/loader.h>

#include <x86/asm.h>

#define CHAINLOAD_STACK		0x2000
#define CHAINLOAD_ADDR		0x7C00

.section .text, "ax", @progbits

/** Drop to real mode and jump to a new boot sector.
 * @param id		ID of boot disk.
 * @param part		Address of partition table entry. */
FUNCTION_START(chain_loader_enter)
	/* Save arguments. */
	movl	4(%esp), %edx
	movl	8(%esp), %esi

	cli

	/* Switch to the real-mode stack. */
	movl	$CHAINLOAD_STACK, %esp

	/* Switch to the BIOS IVT. */
	lidt	(bios_idtp)

	/* Jump into the real mode code segment. */
	ljmp	$SEGMENT_CS16, $1f - LOADER_LOAD_ADDR

	/* Clear the PE (Protection Enable) bit of CR0. */
1:	movl	%cr0, %eax
	andl	$~(1<<0), %eax
	movl	%eax, %cr0

	/* Set correct segments. */
	.byte 0x66
	ljmp	$(LOADER_LOAD_ADDR >> 4), $2f - LOADER_LOAD_ADDR
.code16
2:	mov	$0, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	sti

	/* Restore the console to a decent state. Set display page to the first
	 * and move the cursor to (0, 0). */
	movw	$0x0500, %ax
	int	$0x10
	movw	$0x0200, %ax
	xorw	%bx, %bx
	xorw	%dx, %dx
	int	$0x10

	/* Try to disable the A20 line. */
	call	disable_a20

	/* Jump to the loader. */
	ljmp	$0, $CHAINLOAD_ADDR
FUNCTION_END(chain_loader_enter)

/** Test whether the A20 line is enabled.
 * @note		Partially based on code found at
 *			http://wiki.osdev.org/A20_Line
 * @return		AX = 1 if enabled, AX = 0 if not. */
PRIVATE_FUNCTION_START(test_a20)
.code16
	/* Iterate checking to give time for enabling attempts we make to take
	 * effect. */
	push	%cx
	mov	$0x1000, %cx

	/* Set segment registers and addresses to compare. */
1:	xor	%ax, %ax
	mov	%ax, %fs
	not	%ax
	mov	%ax, %gs
	mov	$0x1000, %di
	mov	$0x1010, %si

	/* Write different values to the two addresses, and compare. If they
	 * both equal the second value written, the A20 line is not enabled. */
	movb	$0x00, %fs:(%di)
	outb	%al, $0x80
	movb	$0xFF, %gs:(%si)
	outb	%al, $0x80
	cmpb	$0xFF, %fs:(%di)
	je	2f
	loop	1b
	mov	$1, %ax
	pop	%cx
	ret
2:	xor	%ax, %ax
	pop	%cx
	ret
FUNCTION_END(test_a20)

/** Disable the A20 line. */
PRIVATE_FUNCTION_START(disable_a20)
	mov	$0x100, %cx
.Lbegin:
	/* First try using a BIOS interrupt. */
	mov	$0x2400, %ax
	int	$0x15
	call	test_a20
	test	%ax, %ax
	jnz	.Ldisable_fast
	ret
.Ldisable_fast:
	/* Try the fast A20 mechanism. */
	inb	$0x92, %al
	andb	$((1<<0) | (1<<1)), %al
	outb	%al, $0x92
	call	test_a20
	test	%ax, %ax
	jnz	.Lfail
	ret
.Lfail:
	loop	.Lbegin

	/* Don't error or anything on failure, on some machines A20 is always
	 * enabled. */
	ret
FUNCTION_END(disable_a20)
