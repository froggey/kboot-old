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
 * @brief		x86 MMU functions.
 */

#include <arch/page.h>

#include <x86/cpu.h>
#include <x86/mmu.h>

#include <lib/string.h>

#include <assert.h>
#include <loader.h>
#include <memory.h>
#include <mmu.h>

/** Allocate a paging structure. */
static phys_ptr_t allocate_structure(mmu_context_t *ctx) {
	phys_ptr_t addr;

	phys_memory_alloc(PAGE_SIZE, PAGE_SIZE, 0, 0x100000000ULL,
		ctx->phys_type, 0, &addr);
	memset((void *)P2V(addr), 0, PAGE_SIZE);
	return addr;
}

/** Get a page directory from a 64-bit context.
 * @param ctx		Context to get from.
 * @param virt		Virtual address to get for.
 * @return		Address of page directory. */
static uint64_t *get_pdir64(mmu_context_t *ctx, uint64_t virt) {
	uint64_t *pml4, *pdp;
	phys_ptr_t addr;
	int pml4e, pdpe;

	pml4 = (uint64_t *)P2V(ctx->cr3);

	/* Get the page directory pointer number. A PDP covers 512GB. */
	pml4e = (virt & 0x0000FFFFFFFFF000) / 0x8000000000;
	if(!(pml4[pml4e] & X86_PTE_PRESENT)) {
		addr = allocate_structure(ctx);
		pml4[pml4e] = addr | X86_PTE_PRESENT | X86_PTE_WRITE;
	}

	/* Get the PDP from the PML4. */
	pdp = (uint64_t *)P2V((ptr_t)(pml4[pml4e] & 0x000000FFFFFFF000LL));

	/* Get the page directory number. A page directory covers 1GB. */
	pdpe = (virt % 0x8000000000) / 0x40000000;
	if(!(pdp[pdpe] & X86_PTE_PRESENT)) {
		addr = allocate_structure(ctx);
		pdp[pdpe] = addr | X86_PTE_PRESENT | X86_PTE_WRITE;
	}

	/* Return the page directory address. */
	return (uint64_t *)P2V((ptr_t)(pdp[pdpe] & 0x000000FFFFFFF000LL));
}

/** Map a large page in a 64-bit context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_large64(mmu_context_t *ctx, uint64_t virt, uint64_t phys) {
	uint64_t *pdir;
	int pde;

	assert(!(virt % 0x200000));
	assert(!(phys % 0x200000));

	pdir = get_pdir64(ctx, virt);
	pde = (virt % 0x40000000) / 0x200000;
	pdir[pde] = phys | X86_PTE_PRESENT | X86_PTE_WRITE | X86_PTE_LARGE;
}

/** Map a small page in a 64-bit context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_small64(mmu_context_t *ctx, uint64_t virt, uint64_t phys) {
	uint64_t *pdir, *ptbl;
	phys_ptr_t addr;
	int pde, pte;

	assert(!(virt % PAGE_SIZE));
	assert(!(phys % PAGE_SIZE));

	pdir = get_pdir64(ctx, virt);

	/* Get the page directory entry number. */
	pde = (virt % 0x40000000) / 0x200000;
	if(!(pdir[pde] & X86_PTE_PRESENT)) {
		addr = allocate_structure(ctx);
		pdir[pde] = addr | X86_PTE_PRESENT | X86_PTE_WRITE;
	}

	/* Get the page table from the page directory. */
	ptbl = (uint64_t *)P2V((ptr_t)(pdir[pde] & 0x000000FFFFFFF000LL));

	/* Map the page. */
	pte = (virt % 0x200000) / PAGE_SIZE;
	ptbl[pte] = phys | X86_PTE_PRESENT | X86_PTE_WRITE;
}

/** Create a mapping in a 64-bit MMU context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to.
 * @param size		Size of the mapping to create.
 * @return		Whether created successfully. */
static bool mmu_map64(mmu_context_t *ctx, uint64_t virt, uint64_t phys, uint64_t size) {
	uint64_t i;

	/* Map using large pages where possible. To do this, align up to a 2MB
	 * boundary using small pages, map anything possible with large pages,
	 * then do the rest using small pages. If virtual and physical addresses
	 * are at different offsets from a large page boundary, we cannot map
	 * using large pages. */
	if((virt % 0x200000) == (phys % 0x200000)) {
		while(virt % 0x200000 && size) {
			map_small64(ctx, virt, phys);
			virt += PAGE_SIZE;
			phys += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		while(size / 0x200000) {
			map_large64(ctx, virt, phys);
			virt += 0x200000;
			phys += 0x200000;
			size -= 0x200000;
		}
	}

	/* Map whatever remains. */
	for(i = 0; i < size; i += PAGE_SIZE)
		map_small64(ctx, virt + i, phys + i);

	return true;
}

/** Map a large page in a 32-bit context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_large32(mmu_context_t *ctx, uint32_t virt, uint32_t phys) {
	uint32_t *pdir;
	int pde;

	assert(!(virt % 0x400000));
	assert(!(phys % 0x400000));

	pdir = (uint32_t *)P2V(ctx->cr3);
	pde = virt / 0x400000;
	pdir[pde] = phys | X86_PTE_PRESENT | X86_PTE_WRITE | X86_PTE_LARGE;
}

/** Map a small page in a 32-bit context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to. */
static void map_small32(mmu_context_t *ctx, uint32_t virt, uint32_t phys) {
	uint32_t *pdir, *ptbl;
	phys_ptr_t addr;
	int pde, pte;

	assert(!(virt % PAGE_SIZE));
	assert(!(phys % PAGE_SIZE));

	pdir = (uint32_t *)P2V(ctx->cr3);

	/* Get the page directory entry number. */
	pde = virt / 0x400000;
	if(!(pdir[pde] & X86_PTE_PRESENT)) {
		addr = allocate_structure(ctx);
		pdir[pde] = addr | X86_PTE_PRESENT | X86_PTE_WRITE;
	}

	/* Get the page table from the page directory. */
	ptbl = (uint32_t *)P2V((ptr_t)(pdir[pde] & 0xFFFFF000));

	/* Map the page. */
	pte = (virt % 0x400000) / PAGE_SIZE;
	ptbl[pte] = phys | X86_PTE_PRESENT | X86_PTE_WRITE;
}

/** Create a mapping in a 32-bit MMU context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to.
 * @param size		Size of the mapping to create.
 * @return		Whether created successfully. */
static bool mmu_map32(mmu_context_t *ctx, uint32_t virt, uint32_t phys, uint32_t size) {
	uint32_t i;

	/* Same as mmu_map64(). We're in non-PAE mode so large pages are 4MB.
	 * FIXME: Only do if PSE is supported. */
	if((virt % 0x400000) == (phys % 0x400000)) {
		while(virt % 0x400000 && size) {
			map_small32(ctx, virt, phys);
			virt += PAGE_SIZE;
			phys += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		while(size / 0x400000) {
			map_large32(ctx, virt, phys);
			virt += 0x400000;
			phys += 0x400000;
			size -= 0x400000;
		}
	}

	/* Map whatever remains. */
	for(i = 0; i < size; i += PAGE_SIZE)
		map_small32(ctx, virt + i, phys + i);

	return true;
}

/** Create a mapping in an MMU context.
 * @param ctx		Context to map in.
 * @param virt		Virtual address to map.
 * @param phys		Physical address to map to.
 * @param size		Size of the mapping to create.
 * @return		Whether created successfully. */
bool mmu_map(mmu_context_t *ctx, target_ptr_t virt, phys_ptr_t phys, target_size_t size) {
	if(virt % PAGE_SIZE || phys % PAGE_SIZE || size % PAGE_SIZE)
		return false;

	if(ctx->is64) {
		return mmu_map64(ctx, virt, phys, size);
	} else {
		if(phys >= 0x100000000LL || (phys + size) > 0x100000000LL) {
			return false;
		} else if(virt >= 0x100000000LL || (virt + size) > 0x100000000LL) {
			return false;
		}

		return mmu_map32(ctx, virt, phys, size);
	}
}

/** Create a new MMU context.
 * @param target	Target operation mode definition.
 * @param phys_type	Physical memory type to use when allocating tables.
 * @return		Pointer to context. */
mmu_context_t *mmu_context_create(target_type_t target, unsigned phys_type) {
	mmu_context_t *ctx;

	ctx = kmalloc(sizeof(*ctx));
	ctx->is64 = target == TARGET_TYPE_64BIT;
	ctx->phys_type = phys_type;
	ctx->cr3 = allocate_structure(ctx);
	return ctx;
}

/** Set bytes in an area of memory. The area must not cross a page boundary.
 * @param ctx		Context to use.
 * @param addr		Virtual address to write to, must be mapped.
 * @param value		Value to write.
 * @param size		Number of bytes to write. */
void mmu_memset(mmu_context_t *ctx, target_ptr_t addr, uint8_t value, target_size_t size) {
	uint64_t *pdir, *ptbl;
	ptr_t page;
	int pde, pte;

	assert(ctx->is64);
	assert(addr / PAGE_SIZE == (addr + size) / PAGE_SIZE);

	pdir = get_pdir64(ctx, addr);

	/* Get the page directory entry number. */
	pde = (addr % 0x40000000) / 0x200000;

	/* Get the page table from the page directory. */
	assert(pdir[pde] & X86_PTE_PRESENT);
	ptbl = (uint64_t *)P2V((ptr_t)(pdir[pde] & 0x000000FFFFFFF000LL));

	/* Get the page table entry number. */
	pte = (addr % 0x200000) / PAGE_SIZE;

	/* Get the page from the page table. */
	assert(ptbl[pte] & X86_PTE_PRESENT);
	page = P2V((ptr_t)(ptbl[pte] & 0x000000FFFFFFF000LL));

	memset((void *)(page + addr % PAGE_SIZE), value, size);
}

/** Set bytes in an area of memory. The area must not cross a page boundary.
 * @param ctx		Context to use.
 * @param addr		Virtual address to write to, must be mapped.
 * @param source	Memory to read from.
 * @param size		Number of bytes to write. */
void mmu_memcpy_to(mmu_context_t *ctx, target_ptr_t addr, const void *source, target_size_t size) {
	uint64_t *pdir, *ptbl;
	ptr_t page;
	int pde, pte;

	assert(ctx->is64);
	assert(addr / PAGE_SIZE == (addr + size) / PAGE_SIZE);

	pdir = get_pdir64(ctx, addr);

	/* Get the page directory entry number. */
	pde = (addr % 0x40000000) / 0x200000;

	/* Get the page table from the page directory. */
	assert(pdir[pde] & X86_PTE_PRESENT);
	ptbl = (uint64_t *)P2V((ptr_t)(pdir[pde] & 0x000000FFFFFFF000LL));

	/* Get the page table entry number. */
	pte = (addr % 0x200000) / PAGE_SIZE;

	/* Get the page from the page table. */
	assert(ptbl[pte] & X86_PTE_PRESENT);
	page = P2V((ptr_t)(ptbl[pte] & 0x000000FFFFFFF000LL));

	memcpy((void *)(page + addr % PAGE_SIZE), source, size);
}

/** Read bytes from an area of memory. The area must not cross a page boundary.
 * @param ctx		Context to use.
 * @param dest		Memory to write to.
 * @param addr		Virtual address to read from, must be mapped.
 * @param size		Number of bytes to read. */
void mmu_memcpy_from(mmu_context_t *ctx, void *dest, target_ptr_t addr, target_size_t size) {
	uint64_t *pdir, *ptbl;
	ptr_t page;
	int pde, pte;

	assert(ctx->is64);
	assert(addr / PAGE_SIZE == (addr + size) / PAGE_SIZE);

	pdir = get_pdir64(ctx, addr);

	/* Get the page directory entry number. */
	pde = (addr % 0x40000000) / 0x200000;

	/* Get the page table from the page directory. */
	assert(pdir[pde] & X86_PTE_PRESENT);
	ptbl = (uint64_t *)P2V((ptr_t)(pdir[pde] & 0x000000FFFFFFF000LL));

	/* Get the page table entry number. */
	pte = (addr % 0x200000) / PAGE_SIZE;

	/* Get the page from the page table. */
	assert(ptbl[pte] & X86_PTE_PRESENT);
	page = P2V((ptr_t)(ptbl[pte] & 0x000000FFFFFFF000LL));

	memcpy(dest, (void *)(page + addr % PAGE_SIZE), size);
}
