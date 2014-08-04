/*
 * Copyright (C) 2014 Henry Harrington
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
 * @brief		Mezzanine loader.
 *
 * This file implements the 'mezzanine' command for loading a Mezzanine image.
 *
 * The 'mezzanine' command is used as follows:
 *
 *   mezzanine "<device name>"
 *
 * @todo		Modules
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <pc/bios.h>
#include <pc/memory.h>
#include <pc/vbe.h>

#include <x86/mmu.h>

#include <assert.h>
#include <device.h>
#include <disk.h>
#include <loader.h>
#include <memory.h>
#include <mmu.h>
#include <ui.h>

typedef struct mezzanine_extent {
	uint64_t store_base;
	uint64_t virtual_base;
	uint64_t size;
	uint64_t flags;
} __packed mezzanine_extent_t;

/* On-disk image header. */
typedef struct mezzanine_header {
	uint8_t magic[16];		/* +0 */
	uint8_t uuid[16];		/* +16 */
	uint16_t protocol_minor;	/* +32 */
	uint16_t protocol_major;	/* +34 */
	uint32_t n_extents;		/* +36 */
	uint64_t entry_fref;		/* +40 */
	uint64_t initial_process;	/* +48 */
	uint64_t _pad1;			/* +56 */
	mezzanine_extent_t extents[64];	/* +64 */
} __packed mezzanine_header_t;

static const char mezzanine_magic[] = "\x00MezzanineImage\x00";
static const uint16_t mezzanine_protocol_major = 0;
static const uint16_t mezzanine_protocol_minor = 0;
// FIXME: Duplicated in enter.S
static const uint64_t mezzanine_physical_map_address = 0xFFFF800000000000ull;
static const uint64_t mezzanine_physical_map_size = 0x8000000000ull;

/** Structure containing Mezzanine image loader state. */
typedef struct mezzanine_loader {
	disk_t *disk;			/**< Image device. */
	char *device_name;		/**< Image device name. */

	mezzanine_header_t header;
} mezzanine_loader_t;

extern void __noreturn mezzanine_arch_enter(phys_ptr_t transition_pml4, phys_ptr_t pml4, uint64_t entry_fref, uint64_t initial_process);

static void generate_pmap(mmu_context_t *mmu) {
	// Iterate the E820 memory map to generate the physical mapping region.
	// Only memory mentioned by the E820 map is mapped here. Device memory, etc is
	// left for the OS to detect and map.

	e820_entry_t *mmap = (e820_entry_t *)BIOS_MEM_BASE;
	size_t count = 0, i;
	bios_regs_t regs;

	bios_regs_init(&regs);

	/* Obtain a memory map using interrupt 15h, function E820h. */
	do {
		regs.eax = 0xE820;
		regs.edx = E820_SMAP;
		regs.ecx = 64;
		regs.edi = BIOS_MEM_BASE + (count * sizeof(e820_entry_t));
		bios_interrupt(0x15, &regs);

		/* If CF is set, the call was not successful. BIOSes are
		 * allowed to return a non-zero continuation value in EBX and
		 * return an error on next call to indicate that the end of the
		 * list has been reached. */
		if(regs.eflags & X86_FLAGS_CF)
			break;

		count++;
	} while(regs.ebx != 0);

	for(i = 0; i < count; i++) {
		// Map liberally, it doesn't matter if free regions overlap with allocated
		// regions. It's more important that the entire region is mapped.
		phys_ptr_t start = ROUND_DOWN(mmap[i].start, PAGE_SIZE);
		phys_ptr_t end = ROUND_UP(mmap[i].start + mmap[i].length, PAGE_SIZE);

		/* What we did above may have made the region too small, warn
		 * and ignore it if this is the case. */
		if(end <= start) {
			continue;
		}

		// Ignore this region if exceeds the map area limit.
		if(start > mezzanine_physical_map_size) {
			continue;
		}

		// Trim it if it starts below the limit.
		if(end > mezzanine_physical_map_size) {
			end = mezzanine_physical_map_size;
		}

		dprintf("mezzanine: Map E820 region %016" PRIx64 "-%016" PRIx64 "\n",
			start, end);

		mmu_map(mmu, mezzanine_physical_map_address + start, start, end - start);
	}
}

/** Load the operating system. */
static __noreturn void mezzanine_loader_load(void) {
	mezzanine_loader_t *loader = current_environ->data;
	mmu_context_t *mmu = mmu_context_create(TARGET_TYPE_64BIT, PHYS_MEMORY_PAGETABLES);
	mmu_context_t *transition = mmu_context_create(TARGET_TYPE_64BIT, PHYS_MEMORY_INTERNAL);

	for(uint32_t i = 0; i < loader->header.n_extents; ++i) {
		// Each extent must be 4k (block) aligned on-disk and
		// 4k (page) aligned in memory.
		dprintf("mezzanine: extent % 2" PRIu32 " %016" PRIx64 " %016" PRIx64 " %08" PRIx64 " %04" PRIx64 "\n",
			i, loader->header.extents[i].store_base, loader->header.extents[i].virtual_base,
			loader->header.extents[i].size, loader->header.extents[i].flags);
		if(loader->header.extents[i].store_base % 4096 ||
		   loader->header.extents[i].virtual_base % PAGE_SIZE ||
		   loader->header.extents[i].size % PAGE_SIZE) {
			boot_error("Extent %" PRIu32 " is misaligned", i);
		}

		// Allocate physical memory. If the extent if 2M aligned, then
		// use 2M pages, otherwise fall back to 4k pages.
		bool use_large_pages = !(loader->header.extents[i].virtual_base % 0x200000 ||
					 loader->header.extents[i].size % 0x200000);
		phys_ptr_t phys_addr;
		phys_memory_alloc(loader->header.extents[i].size, // size
				  use_large_pages ? 0x200000 : 0x1000, // alignment
				  0, 0, // min/max address
				  PHYS_MEMORY_ALLOCATED, // type
				  0, // flags
				  &phys_addr);

		if(!disk_read(loader->disk,
			      (void *)P2V(phys_addr),
			      loader->header.extents[i].size,
			      loader->header.extents[i].store_base)) {
			boot_error("Could not read extent %" PRIu32, i);
		}

		// Map this entry in.
		mmu_map(mmu, loader->header.extents[i].virtual_base, phys_addr,
			loader->header.extents[i].size);
	}

	loader_preboot();

	generate_pmap(mmu);

	// Generate the page tables used for transitioning from identity mapping
	// to the final page tables.
	// The loader must be identity mapped, and mapped in the physical region.
	ptr_t loader_start = ROUND_DOWN((ptr_t)__start, PAGE_SIZE);
	ptr_t loader_size = ROUND_UP((ptr_t)__end - (ptr_t)__start, PAGE_SIZE);
	mmu_map(transition, loader_start, loader_start, loader_size);
	mmu_map(transition, mezzanine_physical_map_address + loader_start, loader_start, loader_size);

	dprintf("mezzanine: Starting system...\n");
	mezzanine_arch_enter(transition->cr3, mmu->cr3, loader->header.entry_fref, loader->header.initial_process);
}

#if CONFIG_KBOOT_UI

/** Return a window for configuring the OS.
 * @return		Pointer to configuration window. */
static ui_window_t *mezzanine_loader_configure(void) {
	return NULL;
}

#endif

/** Mezzanine loader type. */
static loader_type_t mezzanine_loader_type = {
	.load = mezzanine_loader_load,
#if CONFIG_KBOOT_UI
	.configure = mezzanine_loader_configure,
#endif
};

/** Load a Mezzanine image.
 * @param args		Command arguments.
 * @return		Whether completed successfully. */
static bool config_cmd_mezzanine(value_list_t *args) {
	mezzanine_loader_t *data;

	if((args->count != 1) || args->values[0].type != VALUE_TYPE_STRING) {
		dprintf("config: mezzanine: invalid arguments\n");
		return false;
	}

	device_t *device = device_lookup(args->values[0].string);

	/* Verify the device exists and is compatible (disks only, currently). */
	if(!device || device->type != DEVICE_TYPE_DISK) {
		dprintf("mezzanine: Invalid or unsupported device.\n");
		return false;
	}

	data = kmalloc(sizeof *data);
	data->device_name = kstrdup(args->values[0].string);
	data->disk = (disk_t *)device;

	/* Read in the header. */
	if(!disk_read(data->disk, &data->header, sizeof(mezzanine_header_t), 0)) {
		dprintf("mezzanine: IO error, unable to read header.\n");
		goto fail;
	}

	if(memcmp(data->header.magic, mezzanine_magic, 16) != 0) {
		dprintf("mezzanine: Not a mezzanine image, bad header.\n");
		goto fail;
	}

	if(data->header.protocol_major != mezzanine_protocol_major) {
		dprintf("mezzanine: Unsupported protocol major %" PRIu8 ".\n",
			data->header.protocol_major);
		goto fail;
	}

	// Major protocol 0 is for development. The minor must match exactly.
	// Major protocol version above 0 are release version and are backwards
	// compatible at the minor level.
	if((data->header.protocol_major == 0 && data->header.protocol_minor != mezzanine_protocol_minor) ||
	   (data->header.protocol_major != 0 && data->header.protocol_minor > mezzanine_protocol_minor)) {
		dprintf("mezzanine: Unsupported protocol minor %" PRIu8 ".\n",
			data->header.protocol_minor);
		goto fail;
	}

	dprintf("mezzanine: Loading image %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x on device %s with protocol version %" PRIu8 ".%" PRIu8 "\n",
		data->header.uuid[0], data->header.uuid[1], data->header.uuid[2],
		data->header.uuid[3], data->header.uuid[4], data->header.uuid[5],
		data->header.uuid[6], data->header.uuid[7], data->header.uuid[8],
		data->header.uuid[9], data->header.uuid[10], data->header.uuid[11],
		data->header.uuid[12], data->header.uuid[13], data->header.uuid[14],
		data->header.uuid[15],
		data->device_name,
		data->header.protocol_major, data->header.protocol_minor);

	dprintf("mezzanine: Entry fref at %08" PRIx64 ". Initial process at %08" PRIx64 ".\n",
		data->header.entry_fref, data->header.initial_process);


	for(uint32_t i = 0; i < data->header.n_extents; ++i) {
		// Each extent must be 4k (block) aligned on-disk and
		// 4k (page) aligned in memory.
		dprintf("mezzanine: extent % 2" PRIu32 " %016" PRIx64 " %016" PRIx64 " %08" PRIx64 " %04" PRIx64 "\n",
			i,
			data->header.extents[i].store_base, data->header.extents[i].virtual_base,
			data->header.extents[i].size, data->header.extents[i].flags);
	}

	current_environ->loader = &mezzanine_loader_type;
	current_environ->data = data;

	return true;
fail:
	kfree(data->device_name);
	kfree(data);
	return false;
}

BUILTIN_COMMAND("mezzanine", config_cmd_mezzanine);
