/*
 * Copyright (C) 2012 Alex Smith
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
 * @brief		KBoot loader definitions.
 */

#ifndef __LOADERS_KBOOT_H
#define __LOADERS_KBOOT_H

#include <lib/allocator.h>
#include <lib/list.h>

#include <elf.h>
#include <kboot.h>
#include <mmu.h>
#include <target.h>
#include <ui.h>

/** Data for the KBoot loader. */
typedef struct kboot_loader {
	file_handle_t *kernel;		/**< Handle to the kernel image. */
	value_t modules;		/**< Modules to load. */

	/** Kernel image information. */
	target_type_t target;		/**< Target operation mode of the kernel. */
	list_t itags;			/**< Image tag list. */
	kboot_itag_image_t *image;	/**< Image definition tag. */
	uint32_t log_magic;		/**< Magic number for the log buffer. */

	/** Environment for the kernel. */
	target_ptr_t entry;		/**< Kernel entry point. */
	phys_ptr_t tags_phys;		/**< Physical address of tag list. */
	target_ptr_t tags_virt;		/**< Virtual address of tag list. */
	mmu_context_t *mmu;		/**< MMU context for the kernel. */
	allocator_t alloc;		/**< Virtual address space allocator. */
	list_t mappings;		/**< Virtual memory mapping information. */
	target_ptr_t stack_virt;	/**< Base of stack set up for the kernel. */
	target_ptr_t stack_size;	/**< Size of stack set up for the kernel. */
	mmu_context_t *transition;	/**< Kernel transition address space. */
	phys_ptr_t trampoline_phys;	/**< Page containing kernel entry trampoline. */
	target_ptr_t trampoline_virt;	/**< Virtual address of trampoline page. */

	#if CONFIG_KBOOT_UI
	ui_window_t *config;		/**< Configuration window. */
	#endif
} kboot_loader_t;

/** Image tag header structure. */
typedef struct kboot_itag {
	list_t header;			/**< List header. */
	uint32_t type;			/**< Type of the tag. */
} kboot_itag_t;

/** Iterate over all tags of a certain type in the KBoot image tag list.
 * @note		Hurray for language abuse. */
#define KBOOT_ITAG_ITERATE(_loader, _type, _vtype, _vname) \
	LIST_FOREACH(&(_loader)->itags, __##_vname) \
		for(_vtype *_vname = (_vtype *)&list_entry(__##_vname, kboot_itag_t, header)[1]; \
			list_entry(__##_vname, kboot_itag_t, header)->type == _type && _vname; \
			_vname = NULL)

/** Find a tag in the image tag list.
 * @param loader	Loader to find in.
 * @param type		Type of tag to find.
 * @return		Pointer to tag, or NULL if not found. */
static inline void *kboot_itag_find(kboot_loader_t *loader, uint32_t type) {
	kboot_itag_t *itag;

	LIST_FOREACH(&loader->itags, iter) {
		itag = list_entry(iter, kboot_itag_t, header);
		if(itag->type == type)
			return &itag[1];
	}

	return NULL;
}

/** ELF note type (structure is the same for both ELF32 and ELF64). */
typedef Elf32_Note elf_note_t;

/** KBoot ELF note iteration callback.
 * @param note		Note header.
 * @param desc		Note data.
 * @param loader	KBoot loader data structure.
 * @return		Whether to continue iteration. */
typedef bool (*kboot_note_cb_t)(elf_note_t *note, void *desc, kboot_loader_t *loader);

extern void *kboot_allocate_tag(kboot_loader_t *loader, uint32_t type, size_t size);

extern kboot_vaddr_t kboot_allocate_virtual(kboot_loader_t *loader, kboot_paddr_t phys,
	kboot_vaddr_t size);
extern void kboot_map_virtual(kboot_loader_t *loader, kboot_vaddr_t addr,
	kboot_paddr_t phys, kboot_vaddr_t size);

extern bool kboot_elf_note_iterate(kboot_loader_t *loader, kboot_note_cb_t cb);
extern void kboot_elf_load_kernel(kboot_loader_t *loader, kboot_itag_load_t *load);
extern void kboot_elf_load_sections(kboot_loader_t *loader);

extern void kboot_arch_check(kboot_loader_t *loader);
extern void kboot_arch_load_params(kboot_loader_t *loader, kboot_itag_load_t *load);
extern void kboot_arch_setup(kboot_loader_t *loader);
extern void kboot_arch_enter(kboot_loader_t *loader) __noreturn;

extern void kboot_platform_video_init(kboot_loader_t *loader);
extern void kboot_platform_setup(kboot_loader_t *loader);

#endif /* __LOADERS_KBOOT_H */
