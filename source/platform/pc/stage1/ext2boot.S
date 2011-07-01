/*
 * Copyright (C) 2011 Alex Smith
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
 * @brief		Ext2/3/4 boot sector.
 *
 * This code will load the loader binary from an Ext2/3/4 filesystem. It is
 * intended to be written to the first 1024 bytes of a partition and used as
 * the partition's boot sector.
 *
 * Since the MBR only loads 512 bytes for us, we are split into 2 sections, and
 * we load the second half early on in the main function.
 *
 * The start LBA of the boot partition should be written to offset 502. The
 * default value is 63, i.e. the first partition on the disk.
 *
 * Current limitations:
 *  - Only uses INT13 Extensions. This is also the case with the loader itself,
 *    if anyone feels like adding support for the legacy interface, go ahead.
 */

#include <platform/system.h>
#include <x86/asm.h>

#include "../../../fs/ext2.h"

/** Various definitions. */
#define STACK_OFFSET		0x6000		/**< Address of the stack. */

.section .text
.code16

/** Main function. */
FUNCTION_START(_start)
	/* Set up the stack and other segment registers. */
	xorw	%ax, %ax
	movw	%ax, %ss
	movw	$STACK_OFFSET, %sp
	movw	%ax, %ds
	movw	%ax, %es

	movw	$hello, %si
	jmp	error
FUNCTION_END(_start)

/** Display an error string on the screen and halt.
 * @param %ds:%si	String to display. */
PRIVATE_FUNCTION_START(error)
	cld
1:	lodsb
	test	%al, %al
	jz	2f
	movb	$0x0E, %ah
	movw	$1, %bx
	int	$0x10
	jmp	1b
2:	hlt
FUNCTION_END(error)

/** Error strings. */
hello:			.asciz "Hello, World!"

/** Disk address packet. */
.align 4
dap_size:		.byte 0x10
dap_reserved1:		.byte 0
dap_count:		.byte 0
dap_reserved2:		.byte 0
dap_buffer_offset:	.word 0
dap_buffer_segment:	.word (LOADER_LOAD_ADDR >> 4)
dap_lba_low:		.long 0
dap_lba_high:		.long 0

/* Pad up to the end of the first 512 bytes, with space for the partition LBA
 * and the boot signature at the end. */
. = _start + 0x1F6
partition_lba_low:	.long 63
partition_lba_high:	.long 0
.word 0xAA55

/** Pad up to 1KB. */
. = _start + 0x3FF
.byte 0
