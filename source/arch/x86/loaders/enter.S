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

#define ENTRY_ARGS_TRANSITION_CR3	0
#define ENTRY_ARGS_VIRT			8
#define ENTRY_ARGS_KERNEL_CR3		16
#define ENTRY_ARGS_SP			24
#define ENTRY_ARGS_ENTRY		32
#define ENTRY_ARGS_TAGS			40
#define ENTRY_ARGS_TRAMPOLINE		48

/** Enter a 64-bit KBoot kernel.
 * @param args		Entry arguments structure. */
FUNCTION_START(kboot_arch_enter64)
.code32
	/* Store arguments address in EDI. */
	movl	4(%esp), %edi

	/* Enable PAE. */
	movl	%cr4, %eax
	orl	$X86_CR4_PAE, %eax
	movl	%eax, %cr4

	/* Point CR3 to the transition PML4. */
	movl	ENTRY_ARGS_TRANSITION_CR3(%edi), %eax
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
	xorl	%eax, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	/* Clear high 32 bits of arguments address. */
	mov	%edi, %edi

	/* Get the trampoline location and jump to it. */
	movq	ENTRY_ARGS_VIRT(%rdi), %rdi
	leaq	ENTRY_ARGS_TRAMPOLINE(%rdi), %rax
	jmp	*%rax

SYMBOL(kboot_trampoline64)
	/* Switch to the real kernel page directory. */
	movq	ENTRY_ARGS_KERNEL_CR3(%rdi), %rax
	movq	%rax, %cr3

	/* Set the stack pointer. */
	movq	ENTRY_ARGS_SP(%rdi), %rsp

	/* Clear the stack frame/RFLAGS. */
	xorq	%rbp, %rbp
	push	$0
	popf

	/* Retrieve tag list address and entry point. */
	movq	ENTRY_ARGS_TAGS(%rdi), %rsi
	movq	ENTRY_ARGS_ENTRY(%rdi), %rax

	/* Call the kernel. */
	movq	$KBOOT_MAGIC, %rdi
	call	*%rax
1:	jmp	1b

SYMBOL(kboot_trampoline64_size)
	.long	. - kboot_trampoline64
FUNCTION_END(kboot_arch_enter64)

/** Enter a 32-bit KBoot kernel.
 * @param args		Entry arguments structure. */
FUNCTION_START(kboot_arch_enter32)
.code32
	/* Store arguments address in EDI. */
	movl	4(%esp), %edi

	/* Enable PSE, the MMU code may have created mappings with large pages. */
	movl	%cr4, %eax
	orl	$X86_CR4_PSE, %eax
	movl	%eax, %cr4

	/* Point CR3 to the transition page directory. */
	movl	ENTRY_ARGS_TRANSITION_CR3(%edi), %eax
	movl	%eax, %cr3

	/* Set PG (Paging Enable). */
	movl	%cr0, %ecx
	orl	$X86_CR0_PG, %ecx
	movl	%ecx, %cr0

	/* Get the trampoline location and jump to it. */
	movl	ENTRY_ARGS_VIRT(%edi), %edi
	leal	ENTRY_ARGS_TRAMPOLINE(%edi), %eax
	jmp	*%eax

SYMBOL(kboot_trampoline32)
	/* Switch to the real kernel page directory. */
	movl	ENTRY_ARGS_KERNEL_CR3(%edi), %eax
	movl	%eax, %cr3

	/* Set the stack pointer. */
	movl	ENTRY_ARGS_SP(%edi), %esp

	/* Clear the stack frame/EFLAGS. */
	xorl	%ebp, %ebp
	push	$0
	popf

	/* Retrieve tag list address and entry point. */
	movl	ENTRY_ARGS_TAGS(%edi), %edx
	movl	ENTRY_ARGS_ENTRY(%edi), %eax

	/* Call the kernel. */
	push	%edx
	push	$KBOOT_MAGIC
	call	*%eax
1:	jmp	1b

SYMBOL(kboot_trampoline32_size)
	.long	. - kboot_trampoline32
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

#if CONFIG_KBOOT_LOADER_MEZZANINE

.section .text, "ax", @progbits

/** Enter a Mezzanine image.
 * @param transition_pml4	Transition PML4.
 * @param pml4			Final PML4.
 * @param entry_fref		Value of the entry fref.
 * @param initial_process	Value of the initial process. */
FUNCTION_START(mezzanine_arch_enter)
.code32
	/* Enable PAE, PGE, and SSE. */
	movl	%cr4, %eax
	orl	$X86_CR4_PAE|X86_CR4_PGE|X86_CR4_OSFXSR|X86_CR4_OSXMMEXCPT, %eax
	movl	%eax, %cr4

	/* Point CR3 to the transition PML4. */
	movl	4(%esp), %eax
	movl	%eax, %cr3

	/* Enable long mode by setting EFER.LME. */
	movl	$X86_MSR_EFER, %ecx
	rdmsr
	orl	$X86_EFER_LME, %eax
	wrmsr

	/* Configure CR0. This will also put us in compatibility mode. */
	movl	$X86_CR0_PE|X86_CR0_MP|X86_CR0_ET|X86_CR0_NE|X86_CR0_WP|X86_CR0_PG, %ecx
	movl	%ecx, %cr0

	/* Jump into the 64-bit code segment. */
	ljmp	$SEGMENT_CS64, $1f
.align 8
.code64
1:
	/* Set data segments. */
	xorl	%eax, %eax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

        /* Fetch the arguments from the stack. */
        movq    20(%esp), %r13   /* entry fref */
        movq    28(%esp), %r8    /* initial process */

	/* Move to the pmap. */
	movq	$0xFFFF800000000000, %rax
        leaq    1f(%rip), %rcx
        addq    %rcx, %rax
	jmp	*%rax
.align 8
1:
	/* Switch to the real kernel page directory. */
        movq    12(%esp), %rax
	movq	%rax, %cr3

	/* Load the initial process into GS. */
        movq    %r8, %rax
        movq    %r8, %rdx
        sarq    $32, %rdx
        movl    $X86_MSR_GSBASE, %ecx
        wrmsr

        /* Load stack from the initial process */
        movq    31(%r8), %rsp   /* +stack-group-offset-control-stack-base+ */
        addq    39(%r8), %rsp   /* +stack-group-offset-control-stack-size+ */

        /* Clear FPU/SSE state. */
        push    $0x1F80         /* All exceptions masked. Flush to zero disabled. Denormals are zero disabled. Round to nearest. */
        ldmxcsr (%rsp)
        addq    $8, %rsp
        fninit

        /* Clear flags. */
	push	$0
	popf

        /* Clear registers. */
        xorl    %eax, %eax
        xorl    %ecx, %ecx
        xorl    %edx, %edx
        xorl    %ebx, %ebx
        xorl    %ebp, %ebp
        xorl    %esi, %esi
        xorl    %edi, %edi
        xorl    %r8d, %r8d
        xorl    %r9d, %r9d
        xorl    %r10d, %r10d
        xorl    %r11d, %r11d
        xorl    %r12d, %r12d
        /* Not r13, holds the entry fref */
        xorl    %r14d, %r14d
        xorl    %r15d, %r15d

        /* Call the entry function. rcx is already set to 0 for a no-args call. */
        call    *15(%r13)  /* +fref-entry-point+ */

        /* Crash if it returns. */
        ud2a
FUNCTION_END(mezzanine_arch_enter)

#endif /* CONFIG_KBOOT_LOADER_MEZZANINE */
