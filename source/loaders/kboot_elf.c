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
 * @param loader	KBoot loader data structure. */
void kboot_elf_load_kernel(kboot_loader_t *loader) {
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT32
	if(loader->target == TARGET_TYPE_32BIT)
		kboot_elf32_load_kernel(loader);
	#endif
	#if CONFIG_KBOOT_HAVE_LOADER_KBOOT64
	if(loader->target == TARGET_TYPE_64BIT)
		kboot_elf64_load_kernel(loader);
	#endif
}
