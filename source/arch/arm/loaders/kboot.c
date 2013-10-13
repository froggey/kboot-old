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
 * @brief		ARM KBoot kernel loader.
 */

#include <arm/mmu.h>

#include <loaders/kboot.h>

#include <platform/loader.h>

#include <elf.h>
#include <kboot.h>
#include <loader.h>
#include <memory.h>

/** Entry arguments for the kernel. */
typedef struct entry_args {
	uint32_t transition_ttbr;	/**< Transition address space. */
	uint32_t virt;			/**< Virtual location of trampoline. */
	uint32_t kernel_ttbr;		/**< Kernel address space. */
	uint32_t sp;			/**< Stack pointer for the kernel. */
	uint32_t entry;			/**< Entry point for kernel. */
	uint32_t tags;			/**< Tag list virtual address. */

	char trampoline[];
} entry_args_t;

extern void kboot_arch_enter32(entry_args_t *args) __noreturn;

extern char kboot_trampoline32[];
extern size_t kboot_trampoline32_size;

/** Check a kernel image and determine the target type.
 * @param loader	KBoot loader data structure. */
void kboot_arch_check(kboot_loader_t *loader) {
	if(!elf_check(loader->kernel, ELFCLASS32, ELFDATA2LSB, ELF_EM_ARM))
		boot_error("Kernel image is not for this architecture");

	loader->target = TARGET_TYPE_32BIT;
}

/** Validate kernel load parameters.
 * @param loader	KBoot loader data structure.
 * @param load		Load image tag. */
void kboot_arch_load_params(kboot_loader_t *loader, kboot_itag_load_t *load) {
	if(!(load->flags & KBOOT_LOAD_FIXED) && !load->alignment) {
		/* Set default alignment parameters. Just try 1MB alignment
		 * as that allows the kernel to be mapped using sections. */
		load->alignment = load->min_alignment = 0x100000;
	}
}

/** Perform architecture-specific setup tasks.
 * @param loader	KBoot loader data structure. */
void kboot_arch_setup(kboot_loader_t *loader) {
	kboot_tag_pagetables_t *tag;
	target_ptr_t virt;
	uint32_t *l1, *l2;
	phys_ptr_t phys;

	/* Allocate a 1MB temporary mapping region for the kernel. */
	if(!allocator_alloc(&loader->alloc, 0x100000, 0x100000, &virt))
		boot_error("Unable to allocate temporary mapping region");

	/* Create a second level table to cover the region. */
	phys_memory_alloc(PAGE_SIZE, PAGE_SIZE, 0, 0, PHYS_MEMORY_PAGETABLES, 0, &phys);
	memset((void *)P2V(phys), 0, PAGE_SIZE);

	/* Insert it into the first level table, then point its last entry to
	 * itself. */
	l1 = (uint32_t *)P2V(loader->mmu->l1);
	l1[virt / 0x100000] = phys | (1<<0);
	l2 = (uint32_t *)P2V(phys);
	l2[255] = phys | (1<<1) | (1<<4);

	/* Add the pagetables tag. */
	tag = kboot_allocate_tag(loader, KBOOT_TAG_PAGETABLES, sizeof(*tag));
	tag->l1 = loader->mmu->l1;
	tag->mapping = virt;
}

/** Enter a loaded KBoot kernel.
 * @param loader	KBoot loader data structure. */
__noreturn void kboot_arch_enter(kboot_loader_t *loader) {
	entry_args_t *args;

	/* Store information for the entry code. */
	args = (void *)((ptr_t)loader->trampoline_phys);
	args->transition_ttbr = loader->transition->l1;
	args->virt = loader->trampoline_virt;
	args->kernel_ttbr = loader->mmu->l1;
	args->sp = loader->stack_virt + loader->stack_size;
	args->entry = loader->entry;
	args->tags = loader->tags_virt;

	/* Copy the trampoline and call the entry code. */
	memcpy(args->trampoline, kboot_trampoline32, kboot_trampoline32_size);
	kboot_arch_enter32(args);
}
