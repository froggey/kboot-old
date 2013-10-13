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
 * @brief		MIPS Malta loader entry point.
 */

#include <arch/page.h>

#include <mips/asm.h>

#include <platform/loader.h>

/* Ensure that this code is linked in first. */
.section .init.text, "ax", @progbits

/** Main entry point for the loader. */
FUNCTION_START(_start)
.set noreorder
	/*
	 * Registers upon entry are as follows:
	 *  a0 - Number of kernel arguments.
	 *  a1 - Address of the kernel arguments table.
	 *  a2 - Address of the environment variables table.
	 *  a3 - RAM size in bytes.
	 */

	/* Set up the stack. */
	la	$sp, loader_stack

	/* Preserve arguments. */
	move	$s0, $a0
	move	$s1, $a1
	move	$s2, $a2
	move	$s3, $a3

	/* Clear out the BSS section. */
	la	$a0, __bss_start
	move	$a1, $zero
	la	$a2, __bss_end
	subu	$a2, $a2, $a0
	jal	memset
	 nop

	/* Call the main function. */
	move	$a0, $s0
	move	$a1, $s1
	move	$a2, $s2
	move	$a3, $s3

	j	platform_init
FUNCTION_END(_start)

.section .data, "aw", @progbits

/** Stack.
 * @note		Must not be placed in .bss, as it is cleared by memset
 *			whose stack would be overwritten. */
.align PAGE_WIDTH
SYMBOL(loader_stack)
	.fill	PAGE_SIZE
