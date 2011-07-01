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
 * The start LBA (32-bit) of the boot partition should be written to offset 506.
 * The default value is 63, i.e. the first partition on the disk.
 *
 * Current limitations:
 *  - Only uses INT13 Extensions. This is also the case with the loader itself,
 *    if anyone feels like adding support for the legacy interface, go ahead.
 *  - Uses 32-bit LBAs, meaning we only support disks up to 2TB.
 *  - Assumes EXT2_DYNAMIC_REV. The loader itself requires this at the moment,
 *    if that changes we should add support for it here too.
 */

#include <platform/system.h>
#include <x86/asm.h>

#include "../../../fs/ext2.h"

/** Various definitions. */
#define STACK_OFFSET		0x6000		/**< Address of the stack. */

/** Print an error message and halt . */
#define ERROR(s)		\
	movw	$s, %si; \
	jmp	error

.section .text
.code16

/** Main function. */
FUNCTION_START(_start)
	cli

	/* Set up the stack and other segment registers. */
	xorw	%ax, %ax
	movw	%ax, %ss
	movw	$STACK_OFFSET, %sp
	movw	%ax, %ds
	movw	%ax, %es

	/* Save drive ID. */
	mov	%dl, bios_drive_id

	/* Read in our second half and the superblock (3 blocks total). */
	movb	$3, %cl
	xorl	%eax, %eax
	inc	%eax
	movw	$second_half, %di
	call	read_disk_sectors

	/* Check whether this is a valid Ext2 filesystem. */
	movw	(superblock + EXT2_SUPERBLOCK_OFF_MAGIC), %ax
	cmpw	$EXT2_MAGIC, %ax
	je	1f
	ERROR(msg_not_ext2)
1:
	/* Compute and save the block size. Although s_log_block_size is a
	 * 32-bit field in the superblock, in reality it'll never even be
	 * greater than 10, so just treat it as a byte. */
	movl	$1024, %eax
	movb	(superblock + EXT2_SUPERBLOCK_OFF_LOG_BLOCK_SIZE), %cl
	shl	%cl, %eax
	movl	%eax, block_size

	/* Compute and save the group table start LBA. */
	movl	(superblock + EXT2_SUPERBLOCK_OFF_FIRST_DATA_BLOCK), %ecx
	inc	%ecx
	mul	%ecx
	xorl	%edx, %edx
	movl	$512, %ecx
	div	%ecx
	movl	%eax, group_table_offset

	/* Read the root inode. */
	movl	$EXT2_ROOT_INO, %eax
	call	read_inode

	ERROR(hello)
FUNCTION_END(_start)

/** Read a single sector from the boot partition.
 * @param %eax		Start offset.
 * @param %es:%di	Destination buffer. */
PRIVATE_FUNCTION_START(read_disk_sector)
	/* Set count and fall through. */
	movb	$1, %cl
FUNCTION_END(read_disk_sector)

/** Read sectors from the boot partition.
 * @param %cl		Sector count.
 * @param %eax		Start offset.
 * @param %es:%di	Destination buffer. */
PRIVATE_FUNCTION_START(read_disk_sectors)
	pusha

	/* Get the real offset. */
	add	partition_lba, %eax

	/* Initialise DAP. */
	movb	$0x10, dap_size
	movb	$0, dap_reserved1
	xorb	%ch, %ch
	movw	%cx, dap_count
	movw	%di, dap_buffer_offset
	movw	%es, dap_buffer_segment
	movl	%eax, dap_lba_low
	movl	$0, dap_lba_high

	/* Perform the read. */
	movb	bios_drive_id, %dl
	movw	$dap_size, %si
	movb	$0x42, %ah
	int	$0x13
	jc	1f
	popa
	ret
1:	ERROR(msg_read_failed)
FUNCTION_END(read_disk_sectors)

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

/** Read an inode into the inode buffer.
 * @param %eax		Inode to read. */
PRIVATE_FUNCTION_START(read_inode)
	pusha

	/* Compute the group number in EAX, and inode table offset in EDX. */
	dec	%eax
	xorl	%edx, %edx
	divl	(superblock + EXT2_SUPERBLOCK_OFF_INODES_PER_GROUP)
	xchg	%eax, %edx
	movzxw	(superblock + EXT2_SUPERBLOCK_OFF_INODE_SIZE), %ecx
	mul	%ecx
	xchg	%eax, %edx
	push	%edx

	/* Need to read in the group table entry. Work out which sector of the
	 * table it is in (EAX), and the offset into that entry (EDX), and read
	 * it in. */
	movl	$EXT2_GROUP_DESC_SIZE, %ecx
	mul	%ecx
	xorl	%edx, %edx
	movl	$512, %ecx
	div	%ecx
	addl	group_table_offset, %eax
	movw	$temp_buffer, %di
	call	read_disk_sector

	/* Retrieve the inode table block, then work out the sector that the
	 * inode is in (EAX) and the offset into that sector (EDX), and read
	 * it in. */
	movl	(temp_buffer + EXT2_GROUP_DESC_OFF_INODE_TABLE)(%edx), %eax
	mull	block_size
	pop	%edx
	add	%edx, %eax
	xorl	%edx, %edx
	movl	$512, %ecx
	div	%ecx
	movw	$temp_buffer, %di
	call	read_disk_sector

	/* Copy the inode to the inode buffer. */
	cld
	movzxw	(superblock + EXT2_SUPERBLOCK_OFF_INODE_SIZE), %ecx
	mov	$inode, %edi
	mov	$temp_buffer, %esi
	addl	%edx, %esi
	rep movsb

	popa
	ret
FUNCTION_END(read_inode)

/** Error strings. */
msg_read_failed:	.asciz "Read failed"
msg_not_ext2:		.asciz "Unrecognised FS"
hello:			.asciz "H"

/* Pad up to the end of the first 512 bytes, with space for the partition LBA
 * and the boot signature at the end. */
. = _start + 0x1FA
partition_lba:		.long 63
boot_signature:		.word 0xAA55

second_half:



/** Pad up to 1KB. */
. = _start + 0x3FF
.byte 0

.section .bss

/** Superblock. This is read into memory along with our second half. */
superblock:		.fill EXT2_SUPERBLOCK_SIZE

/** Current inode buffer. */
inode:			.fill EXT2_INODE_SIZE

/** Temporary sector buffer. */
temp_buffer:		.fill 512

/** Disk address packet. */
dap_size:		.byte 0
dap_reserved1:		.byte 0
dap_count:		.byte 0
dap_reserved2:		.byte 0
dap_buffer_offset:	.word 0
dap_buffer_segment:	.word 0
dap_lba_low:		.long 0
dap_lba_high:		.long 0

/** Saved BIOS drive ID. */
bios_drive_id:		.byte 0

/** Pre-computed FS information. */
block_size:		.long 0
group_table_offset:	.long 0
