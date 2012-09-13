/*
 * Copyright (C) 2010-2012 Alex Smith
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
 * @brief		x86 kernel entry functions.
 */

#include <arch/page.h>

#include <x86/asm.h>
#include <x86/cpu.h>

#include <platform/loader.h>

#include <kboot.h>

.section .text, "ax", @progbits

#if CONFIG_KBOOT_LOADER_KBOOT

/** Enter a 64-bit KBoot kernel.
 * @param tags		Address of tag list.
 * @param cr3		Value to load into CR3.
 * @param entry		Entry point address. */
FUNCTION_START(kboot_arch_enter64)
.code32
	/* Enable PAE. */
	movl	%cr4, %eax
	orl	$X86_CR4_PAE, %eax
	movl	%eax, %cr4

	/* Point CR3 to the boot PML4. */
	movl	12(%esp), %eax
	movl	%eax, %cr3

	/* Enable long mode by setting EFER.LME. */
	movl	$X86_MSR_EFER, %ecx
	rdmsr
	orl	$X86_EFER_LME, %eax
	wrmsr

	/* Set PG (Paging Enable) to put us in compatibility mode. */
	movl	%cr0, %ecx
	orl	$X86_CR0_PG, %ecx
	movl	%ecx, %cr0

	/* Jump into the 64-bit code segment. */
	ljmp	$SEGMENT_CS64, $.Llmode
.align 8
.code64
.Llmode:
	/* Set data segments. */
	mov	$SEGMENT_DS64, %ax
	mov	%ax, %ss
	xorl	%eax, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs

        /* Clear the high 32 bits of %rsp. */
        mov	%esp, %esp

	/* Retreive the tag list address and entry point. */
	movq	$KBOOT_MAGIC, %rdi
	movq	4(%rsp), %rsi
	movq	16(%rsp), %rax

	/* Set the stack pointer to the top of the stack. */
	addl	$PAGE_SIZE, %esp
	andl	$0xFFFFF000, %esp

	/* Clear the stack frame/RFLAGS. */
	xorq	%rbp, %rbp
	push	$0
	popf

	/* Call the kernel. */
	call	*%rax
FUNCTION_END(kboot_arch_enter64)

/** Enter a 32-bit KBoot kernel.
 * @param tags		Address of tag list.
 * @param cr3		Value to load into CR3.
 * @param entry		Entry point address. */
FUNCTION_START(kboot_arch_enter32)
.code32
	/* Enable PSE, the MMU code may have created mappings with large pages. */
	movl	%cr4, %eax
	orl	$X86_CR4_PSE, %eax
	movl	%eax, %cr4

	/* Point CR3 to the boot page directory. */
	movl	12(%esp), %eax
	movl	%eax, %cr3

	/* Set PG (Paging Enable). */
	movl	%cr0, %ecx
	orl	$X86_CR0_PG, %ecx
	movl	%ecx, %cr0

	/* Retreive the tag list address (phys_ptr_t, 64-bit) and entry point. */
	movl	4(%esp), %edi
	movl	8(%esp), %esi
	movl	16(%esp), %eax

	/* Set the stack pointer to the top of the stack. */
	addl	$PAGE_SIZE, %esp
	andl	$0xFFFFF000, %esp

	/* Clear the stack frame/EFLAGS. */
	xorl	%ebp, %ebp
	push	$0
	popf

	/* Call the kernel. */
	push	%esi
	push	%edi
	push	$KBOOT_MAGIC
	call	*%eax
1:	jmp	1b
FUNCTION_END(kboot_arch_enter32)

#endif /* CONFIG_KBOOT_LOADER_KBOOT */

#if CONFIG_KBOOT_LOADER_LINUX

/** Enter a Linux kernel.
 * @param entry		Address of kernel entry point.
 * @param params	Address of kernel parameters page.
 * @param sp		Stack pointer to use. */
FUNCTION_START(linux_arch_enter)
.code32
	cli

	/* Load the GDT and new segments. */
	lgdt	(linux_gdtp)
	ljmp	$0x10, $2f
2:
	mov	$0x18, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	/* Get the arguments. */
	movl	4(%esp), %eax
	movl	8(%esp), %esi
	movl	12(%esp), %esp

	/* Clear out EFLAGS and other registers required to be 0. */
	xorl	%ebp, %ebp
	xorl	%edi, %edi
	xorl	%ebx, %ebx
	push	$0
	popf

	/* Jump to the kernel entry point. */
	call	*%eax
1:	jmp	1b
FUNCTION_END(linux_arch_enter)

#endif /* CONFIG_KBOOT_LOADER_LINUX */

.section .data, "aw", @progbits

#if CONFIG_KBOOT_LOADER_LINUX

/** GDT pointer. */
linux_gdtp:
        .word .L__gdt_end-__linux_gdt-1
        .long __linux_gdt

/** Global descriptor table. */
__linux_gdt:
	.quad 0x0000000000000000                /**< NULL descriptor (0x00). */
	.quad 0x0000000000000000                /**< NULL descriptor (0x08). */
        .quad 0x00CF9A000000FFFF                /**< 32-bit code     (0x10). */
        .quad 0x00CF92000000FFFF                /**< 32-bit data     (0x18). */
.L__gdt_end:

#endif /* CONFIG_KBOOT_LOADER_LINUX */
