/*
 * Copyright (C) 2011-2012 Alex Smith
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
 * @brief		KBoot ELF loading functions.
 */

#include <lib/utility.h>

#include <loaders/kboot.h>

#include <elf.h>
#include <memory.h>
#include <mmu.h>

/** Allocate memory for the kernel image.
 * @param loader	KBoot loader data structure.
 * @param load		Image load parameters.
 * @param virt_base	Virtual base address.
 * @param virt_end	Virtual end address.
 * @return		Physical address allocated for kernel. */
static phys_ptr_t allocate_kernel(kboot_loader_t *loader, kboot_itag_load_t *load,
	target_ptr_t virt_base, target_ptr_t virt_end)
{
	kboot_tag_core_t *core;
	target_size_t size;
	phys_ptr_t ret;
	size_t align;

	size = ROUND_UP(virt_end - virt_base, PAGE_SIZE);

	/* Try to find some space to load to. Iterate down in powers of 2 unti
	 * we reach the minimum alignment. */
	align = load->alignment;
	while(!phys_memory_alloc(size, align, 0, 0, PHYS_MEMORY_ALLOCATED, PHYS_ALLOC_CANFAIL, &ret)) {
		align >>= 1;
		if(align < load->min_alignment || align < PAGE_SIZE)
			boot_error("You do not have enough memory available");
	}

	dprintf("kboot: loading kernel to 0x%" PRIxPHYS " (alignment: 0x%" PRIxPHYS
		", min_alignment: 0x%" PRIxPHYS ", size: 0x%" PRIx64 ", virt_base: 0x%"
		PRIx64 ")\n", ret, load->alignment, load->min_alignment, size,
		virt_base);

	/* Map in the kernel image. */
	kboot_map_virtual(loader, virt_base, ret, size);

	core = (kboot_tag_core_t *)P2V(loader->tags_phys);
	core->kernel_phys = ret;
	return ret;
}

/** Allocate memory for a single segment.
 * @param loader	KBoot loader data structure.
 * @param load		Image load parameters.
 * @param virt		Virtual load address.
 * @param phys		Physical load address.
 * @param size		Total load size.
 * @param idx		Segment index. */
static void allocate_segment(kboot_loader_t *loader, kboot_itag_load_t *load,
	target_ptr_t virt, phys_ptr_t phys, target_size_t size, size_t idx)
{
	phys_ptr_t ret;

	size = ROUND_UP(size, PAGE_SIZE);

	/* Allocate the exact physical address specified. */
	phys_memory_alloc(size, 0, phys, phys + size, PHYS_MEMORY_ALLOCATED, 0, &ret);

	dprintf("kboot: loading segment %zu to 0x%" PRIxPHYS " (size: 0x%" PRIx64
		", virt: 0x%" PRIx64 ")\n", idx, phys, size, virt);

	/* Map the address range. */
	kboot_map_virtual(loader, virt, phys, size);
}

#if CONFIG_KBOOT_HAVE_LOADER_KBOOT32
# define KBOOT_LOAD_ELF32
# include "kboot_elfxx.h"
# undef KBOOT_LOAD_ELF32
#endif

#if CONFIG_KBOOT_HAVE_LOADER_KBOOT64
# define KBOOT_LOAD_ELF64
# include "kboot_elfxx.h"
# undef KBOOT_LOAD_ELF64
#endif

/** Iterate over KBoot ELF notes.
 * @param loader	KBoot loader data structure.
 * @param cb		Callback function.
 * @return		Whether the file is an ELF file. */
bool kboot_elf_note_iterate(kboot_loader_t *loader, kboot_note_cb_t cb) {
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT32
	if(elf_check(loader->kernel, ELFCLASS32, 0, 0))
		return kboot_elf32_note_iterate(loader, cb);
	#endif
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT64
	if(elf_check(loader->kernel, ELFCLASS64, 0, 0))
		return kboot_elf64_note_iterate(loader, cb);
	#endif

	return false;
}

/** Load an ELF kernel image.
 * @param loader	KBoot loader data structure.
 * @param load		Image load parameters. */
void kboot_elf_load_kernel(kboot_loader_t *loader, kboot_itag_load_t *load) {
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT32
	if(loader->target == TARGET_TYPE_32BIT)
		kboot_elf32_load_kernel(loader, load);
	#endif
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT64
	if(loader->target == TARGET_TYPE_64BIT)
		kboot_elf64_load_kernel(loader, load);
	#endif
}

/** Load additional sections for an ELF kernel image.
 * @param loader	KBoot loader data structure. */
void kboot_elf_load_sections(kboot_loader_t *loader) {
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT32
	if(loader->target == TARGET_TYPE_32BIT)
		kboot_elf32_load_sections(loader);
	#endif
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT64
	if(loader->target == TARGET_TYPE_64BIT)
		kboot_elf64_load_sections(loader);
	#endif
}
