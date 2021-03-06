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
 * @brief		KBoot ELF loading functions.
 */

#ifdef KBOOT_LOAD_ELF64
# define elf_ehdr_t	Elf64_Ehdr
# define elf_phdr_t	Elf64_Phdr
# define elf_shdr_t	Elf64_Shdr
# define elf_addr_t	Elf64_Addr
# define ELF_BITS	64
# define FUNC(name)	kboot_elf64_##name
#else
# define elf_ehdr_t	Elf32_Ehdr
# define elf_phdr_t	Elf32_Phdr
# define elf_shdr_t	Elf32_Shdr
# define elf_addr_t	Elf32_Addr
# define ELF_BITS	32
# define FUNC(name)	kboot_elf32_##name
#endif

/** Iterate over note sections in an ELF file. */
static bool FUNC(note_iterate)(kboot_loader_t *loader, kboot_note_cb_t cb) {
	elf_phdr_t *phdrs;
	elf_note_t *note;
	const char *name;
	size_t i, offset;
	void *buf, *desc;
	elf_ehdr_t ehdr;

	if(!file_read(loader->kernel, &ehdr, sizeof(ehdr), 0))
		return false;

	phdrs = kmalloc(sizeof(*phdrs) * ehdr.e_phnum);
	if(!file_read(loader->kernel, phdrs, ehdr.e_phnum * ehdr.e_phentsize, ehdr.e_phoff))
		return false;

	for(i = 0; i < ehdr.e_phnum; i++) {
		if(phdrs[i].p_type != ELF_PT_NOTE)
			continue;

		buf = kmalloc(phdrs[i].p_filesz);
		if(!file_read(loader->kernel, buf, phdrs[i].p_filesz, phdrs[i].p_offset))
			return false;

		for(offset = 0; offset < phdrs[i].p_filesz; ) {
			note = (elf_note_t *)(buf + offset);
			offset += sizeof(elf_note_t);
			name = (const char *)(buf + offset);
			offset += ROUND_UP(note->n_namesz, 4);
			desc = buf + offset;
			offset += ROUND_UP(note->n_descsz, 4);

			if(strcmp(name, "KBoot") == 0) {
				if(!cb(note, desc, loader)) {
					kfree(buf);
					kfree(phdrs);
					return true;
				}
			}
		}

		kfree(buf);
	}

	kfree(phdrs);
	return true;
}

/** Load an ELF kernel image. */
static void FUNC(load_kernel)(kboot_loader_t *loader, kboot_itag_load_t *load) {
	elf_addr_t virt_base = 0, virt_end = 0;
	phys_ptr_t phys = 0;
	elf_phdr_t *phdrs;
	elf_ehdr_t ehdr;
	ptr_t dest;
	size_t i;

	if(!file_read(loader->kernel, &ehdr, sizeof(ehdr), 0))
		boot_error("Could not read kernel image");

	phdrs = kmalloc(sizeof(*phdrs) * ehdr.e_phnum);
	if(!file_read(loader->kernel, phdrs, ehdr.e_phnum * ehdr.e_phentsize, ehdr.e_phoff))
		boot_error("Could not read kernel image");

	/* If not loading at a fixed location, we allocate a single block of
	 * physical memory to load at. */
	if(!(load->flags & KBOOT_LOAD_FIXED)) {
		/* Calculate the total load size of the kernel. */
		for(i = 0; i < ehdr.e_phnum; i++) {
			if(phdrs[i].p_type != ELF_PT_LOAD)
				continue;

			if(virt_base == 0 || virt_base > phdrs[i].p_vaddr)
				virt_base = phdrs[i].p_vaddr;
			if(virt_end < (phdrs[i].p_vaddr + phdrs[i].p_memsz))
				virt_end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
		}

		phys = allocate_kernel(loader, load, virt_base, virt_end);
	}

	/* Load in the image data. */
	for(i = 0; i < ehdr.e_phnum; i++) {
		if(phdrs[i].p_type != ELF_PT_LOAD)
			continue;

		/* If loading at a fixed location, we have to allocate space. */
		if(load->flags & KBOOT_LOAD_FIXED) {
			allocate_segment(loader, load, phdrs[i].p_vaddr, phdrs[i].p_paddr,
				phdrs[i].p_memsz, i);
			dest = P2V(phdrs[i].p_paddr);
		} else {
			dest = P2V(phys + (phdrs[i].p_vaddr - virt_base));
		}

		if(!file_read(loader->kernel, (void *)dest, phdrs[i].p_filesz, phdrs[i].p_offset))
			boot_error("Could not read kernel image");

		/* Clear BSS sections. */
		memset((void *)(dest + (ptr_t)phdrs[i].p_filesz), 0,
			phdrs[i].p_memsz - phdrs[i].p_filesz);
	}

	loader->entry = ehdr.e_entry;
}

/** Load additional sections from an ELF kernel image. */
static void FUNC(load_sections)(kboot_loader_t *loader) {
	kboot_tag_sections_t *tag;
	kboot_tag_core_t *core;
	elf_shdr_t *shdr;
	elf_ehdr_t ehdr;
	phys_ptr_t addr;
	size_t size, i;
	void *dest;

	if(!file_read(loader->kernel, &ehdr, sizeof(ehdr), 0))
		boot_error("Could not read kernel image");

	size = ehdr.e_shnum * ehdr.e_shentsize;

	tag = kboot_allocate_tag(loader, KBOOT_TAG_SECTIONS, sizeof(*tag) + size);
	tag->num = ehdr.e_shnum;
	tag->entsize = ehdr.e_shentsize;
	tag->shstrndx = ehdr.e_shstrndx;

	if(!file_read(loader->kernel, tag->sections, size, ehdr.e_shoff))
		boot_error("Could not read kernel image");

	core = (kboot_tag_core_t *)P2V(loader->tags_phys);

	/* Iterate through the headers and load in additional loadable sections. */
	for(i = 0; i < ehdr.e_shnum; i++) {
		shdr = (elf_shdr_t *)&tag->sections[i * ehdr.e_shentsize];

		if(shdr->sh_flags & ELF_SHF_ALLOC || shdr->sh_addr || !shdr->sh_size
			|| (shdr->sh_type != ELF_SHT_PROGBITS
				&& shdr->sh_type != ELF_SHT_NOBITS
				&& shdr->sh_type != ELF_SHT_SYMTAB
				&& shdr->sh_type != ELF_SHT_STRTAB)) {
			continue;
		}

		/* Allocate memory to load the section data to. Try to make it
		 * contiguous with the kernel image. */
		phys_memory_alloc(ROUND_UP(shdr->sh_size, PAGE_SIZE), 0,
			core->kernel_phys, 0, PHYS_MEMORY_ALLOCATED, 0,
			&addr);
		shdr->sh_addr = addr;

		/* Load in the section data. */
		dest = (void *)P2V(addr);
		if(shdr->sh_type == ELF_SHT_NOBITS) {
			memset(dest, 0, shdr->sh_size);
		} else {
			if(!file_read(loader->kernel, dest, shdr->sh_size, shdr->sh_offset))
				boot_error("Could not read kernel image");
		}

		dprintf("kboot: loaded ELF section %zu to 0x%" PRIxPHYS " (size: %zu)\n",
			i, addr, (size_t)shdr->sh_size);
	}
}

#undef elf_ehdr_t
#undef elf_phdr_t
#undef elf_shdr_t
#undef elf_addr_t
#undef ELF_BITS
#undef FUNC
