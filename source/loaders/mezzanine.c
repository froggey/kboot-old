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
	uint64_t bump;
	uint64_t flags;
} __packed mezzanine_extent_t;

/* On-disk image header. */
typedef struct mezzanine_header {
	uint8_t magic[16];		/* +0 */
	uint8_t uuid[16];		/* +16 */
	uint16_t protocol_major;	/* +32 */
	uint16_t protocol_minor;	/* +34 */
	uint32_t n_extents;		/* +36 */
	uint64_t entry_fref;		/* +40 */
	uint64_t initial_process;	/* +48 */
	uint64_t nil;			/* +56 */
	uint8_t _pad2[32];		/* +64 */
	uint64_t bml4;   		/* +96 */
	uint64_t freelist_head;		/* +104 */
	mezzanine_extent_t extents[64];	/* +112 */
} __packed mezzanine_header_t;

typedef struct mezzanine_page_info {
	uint64_t flags;
	uint64_t bin;
	uint64_t next;
	uint64_t prev;
} __packed mezzanine_page_info_t;

typedef struct mezzanine_buddy_bin {
	uint64_t first_page;
	uint64_t count;
} __packed mezzanine_buddy_bin_t;

static const char mezzanine_magic[] = "\x00MezzanineImage\x00";
static const uint16_t mezzanine_protocol_major = 0;
static const uint16_t mezzanine_protocol_minor = 13;
// FIXME: Duplicated in enter.S
static const uint64_t mezzanine_physical_map_address = 0xFFFF800000000000ull;
static const uint64_t mezzanine_physical_info_address = 0xFFFF808000000000ull;
static const uint64_t mezzanine_physical_map_size = 0x8000000000ull;
#define mezzanine_n_buddy_bins 32

/* Boot info page */

typedef struct mezzanine_boot_information {
	uint8_t uuid[16];                                          // +0 octets.
	mezzanine_buddy_bin_t buddy_bin[mezzanine_n_buddy_bins];   // +16
	// Video information.
	// Framebuffer size is stride * height, aligned up to page size.
	uint64_t framebuffer_physical_address;                     // +528 fixnum.
	uint64_t framebuffer_width;                                // +536 fixnum, pixels.
	uint64_t framebuffer_pitch;                                // +544 fixnum, bytes.
	uint64_t framebuffer_height;                               // +552 fixnum, pixels.
	uint64_t framebuffer_layout;                               // +560 fixnum.
} __packed mezzanine_boot_information_t;

// Common(ish) layouts. Layouts beyond 32-bit XRGB will be supported in later boot protocols.
#define FRAMEBUFFER_LAYOUT_X8_R8_G8_B8 1  // 32-bit XRGB
//#define FRAMEBUFFER_LAYOUT_R8_G8_B8_X8 2  // 32-bit RGBX
//#define FRAMEBUFFER_LAYOUT_X8_B8_G8_R8 3  // 32-bit XBGR
//#define FRAMEBUFFER_LAYOUT_B8_G8_R8_X8 4  // 32-bit BGRX
//#define FRAMEBUFFER_LAYOUT_X0_R8_G8_B8 5  // 24-bit RGB
//#define FRAMEBUFFER_LAYOUT_X0_B8_G8_R8 6  // 24-bit BGR
//#define FRAMEBUFFER_LAYOUT_X0_R5_G6_B5 7  // 16-bit 565 RGB
//#define FRAMEBUFFER_LAYOUT_X0_B5_G6_R5 8  // 16-bit 565 BGR
//#define FRAMEBUFFER_LAYOUT_X1_R5_G5_B5 9  // 16-bit 555 XRGB
//#define FRAMEBUFFER_LAYOUT_X1_B5_G5_R5 10 // 16-bit 555 XBGR
//#define FRAMEBUFFER_LAYOUT_R5_G5_B5_X1 11 // 16-bit 555 RGBX
//#define FRAMEBUFFER_LAYOUT_B5_G5_R5_X1 12 // 16-bit 555 BGRX

#define MEZZANINE_EXTENT_TYPE_MASK 0x7
#define MEZZANINE_EXTENT_TYPE_PINNED 0x0
#define MEZZANINE_EXTENT_TYPE_PINNED_2G 0x1
#define MEZZANINE_EXTENT_TYPE_DYNAMIC 0x2
#define MEZZANINE_EXTENT_TYPE_DYNAMIC_CONS 0x3
#define MEZZANINE_EXTENT_TYPE_NURSERY 0x4
#define MEZZANINE_EXTENT_TYPE_STACK 0x5
#define MEZZANINE_EXTENT_WIRED 0x8
#define MEZZANINE_EXTENT_LARGE 0x10
#define MEZZANINE_EXTENT_FINISHED 0x20
#define MEZZANINE_EXTENT_FLIPPED 0x40

#define MEZZANINE_STACK_MARK_BIT 0x100000000000

/** Structure containing Mezzanine image loader state. */
typedef struct mezzanine_loader {
	disk_t *disk;			/**< Image device. */
	char *device_name;		/**< Image device name. */

	mezzanine_header_t header;
} mezzanine_loader_t;

extern void __noreturn mezzanine_arch_enter(phys_ptr_t transition_pml4, phys_ptr_t pml4, uint64_t entry_fref, uint64_t initial_process, uint64_t boot_information_location);

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

		// Allocate the information area for these pages.
		// FIXME: This has a bit of a problem with overlap, leaking a few pages.
		phys_ptr_t info_start = ROUND_DOWN((mezzanine_physical_info_address + (start / PAGE_SIZE) * sizeof(mezzanine_page_info_t)), PAGE_SIZE);
		phys_ptr_t info_end = ROUND_UP((mezzanine_physical_info_address + (end / PAGE_SIZE) * sizeof(mezzanine_page_info_t)), PAGE_SIZE);
		phys_ptr_t phys_info_addr;
		phys_memory_alloc(info_end - info_start, // size
				  0x1000, // alignment
				  0, 0, // min/max address
				  PHYS_MEMORY_ALLOCATED, // type
				  0, // flags
				  &phys_info_addr);
		mmu_map(mmu, info_start, phys_info_addr, info_end - info_start);
		memset((void *)P2V(phys_info_addr), 0, info_end - info_start);
	}
}

/// Convert val to a fixnum.
static uint64_t fixnum(int64_t val) {
	return val << 1;
}

#define PAGE_FLAG_FREE 1

static uint64_t page_info_flags(mmu_context_t *mmu, phys_ptr_t page) {
	uint64_t offset = page / PAGE_SIZE;
	uint64_t result;
	mmu_memcpy_from(mmu, &result, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, flags), sizeof result);
	return result;
}

static void set_page_info_flags(mmu_context_t *mmu, phys_ptr_t page, uint64_t value) {
	uint64_t offset = page / PAGE_SIZE;
	mmu_memcpy_to(mmu, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, flags), &value, sizeof value);
}

static uint64_t page_info_bin(mmu_context_t *mmu, phys_ptr_t page) {
	uint64_t offset = page / PAGE_SIZE;
	uint64_t result;
	mmu_memcpy_from(mmu, &result, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, bin), sizeof result);
	return result;
}

static void set_page_info_bin(mmu_context_t *mmu, phys_ptr_t page, uint64_t value) {
	uint64_t offset = page / PAGE_SIZE;
	mmu_memcpy_to(mmu, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, bin), &value, sizeof value);
}

static uint64_t page_info_next(mmu_context_t *mmu, phys_ptr_t page) {
	uint64_t offset = page / PAGE_SIZE;
	uint64_t result;
	mmu_memcpy_from(mmu, &result, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, next), sizeof result);
	return result;
}

static void set_page_info_next(mmu_context_t *mmu, phys_ptr_t page, uint64_t value) {
	uint64_t offset = page / PAGE_SIZE;
	mmu_memcpy_to(mmu, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, next), &value, sizeof value);
}

static uint64_t page_info_prev(mmu_context_t *mmu, phys_ptr_t page) {
	uint64_t offset = page / PAGE_SIZE;
	uint64_t result;
	mmu_memcpy_from(mmu, &result, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, prev), sizeof result);
	return result;
}

static void set_page_info_prev(mmu_context_t *mmu, phys_ptr_t page, uint64_t value) {
	uint64_t offset = page / PAGE_SIZE;
	mmu_memcpy_to(mmu, mezzanine_physical_info_address + offset * sizeof(mezzanine_page_info_t) + offsetof(mezzanine_page_info_t, prev), &value, sizeof value);
}

static phys_ptr_t buddy(int k, phys_ptr_t x) {
	return x ^ ((phys_ptr_t)1 << (k + 12));
}

static void buddy_free_page(mmu_context_t *mmu, mezzanine_boot_information_t *boot_info, uint64_t nil, phys_ptr_t l) {
	int m = mezzanine_n_buddy_bins-1;
	int k = 0;

	while(1) {
		phys_ptr_t p = buddy(k, l);
		if(k == m || // Stop trying to combine at the last bin.
		   // Don't combine if buddy is not free
		   (page_info_flags(mmu, p) & fixnum(PAGE_FLAG_FREE)) != fixnum(PAGE_FLAG_FREE) ||
		   // Only combine when the buddy is in this bin.
		   ((page_info_flags(mmu, p) & fixnum(PAGE_FLAG_FREE)) == fixnum(PAGE_FLAG_FREE) &&
		    page_info_bin(mmu, p) != fixnum(k))) {
			break;
		}
		// remove buddy from avail[k]
		if(boot_info->buddy_bin[k].first_page == fixnum(p / PAGE_SIZE)) {
			boot_info->buddy_bin[k].first_page = page_info_next(mmu, p);
		}
		if(page_info_next(mmu, p) != nil) {
			set_page_info_prev(mmu, page_info_next(mmu, p), page_info_prev(mmu, p));
		}
		if(page_info_prev(mmu, p) != nil) {
			set_page_info_next(mmu, page_info_prev(mmu, p), page_info_next(mmu, p));
		}
		boot_info->buddy_bin[k].count -= fixnum(1);
		k += 1;
		if(p < l) {
			l = p;
		}
	}
	set_page_info_flags(mmu, l, page_info_flags(mmu, l) | fixnum(PAGE_FLAG_FREE));
	set_page_info_bin(mmu, l, fixnum(k));
	set_page_info_next(mmu, l, boot_info->buddy_bin[k].first_page);
	set_page_info_prev(mmu, l, nil);
	if(boot_info->buddy_bin[k].first_page != nil) {
		set_page_info_prev(mmu, boot_info->buddy_bin[k].first_page, fixnum(l));
	}
	boot_info->buddy_bin[k].first_page = fixnum(l / PAGE_SIZE);
	boot_info->buddy_bin[k].count += fixnum(1);
}

static int determine_vbe_mode_layout(vbe_mode_t *mode) {
	switch(mode->info.bits_per_pixel) {
	case 32:
		if(mode->info.red_mask_size == 8 &&
		   mode->info.red_field_position == 16 &&
		   mode->info.green_mask_size == 8 &&
		   mode->info.green_field_position == 8 &&
		   mode->info.blue_mask_size == 8 &&
		   mode->info.blue_field_position == 0) {
			// God's true video mode layout (not 640x480x4, you heathens).
			return FRAMEBUFFER_LAYOUT_X8_R8_G8_B8;
		} else {
			return 0;
		}
	default:
		return 0;
	}
}

static void set_video_mode(mezzanine_boot_information_t *boot_info)
{
	value_t *entry = environ_lookup(current_environ, "video_mode");
	vbe_mode_t *mode = NULL;
	int layout = 0;

	if(entry && entry->type == VALUE_TYPE_STRING) {
		// Decode the video mode, and search for a matching mode.
		uint16_t width = 0, height = 0;
		char *dup, *orig, *tok;
		uint8_t depth = 0;

		dup = orig = kstrdup(entry->string);

		if((tok = strsep(&dup, "x")))
			width = strtol(tok, NULL, 0);
		if((tok = strsep(&dup, "x")))
			height = strtol(tok, NULL, 0);
		if((tok = strsep(&dup, "x")))
			depth = strtol(tok, NULL, 0);

		kfree(orig);

		if(width && height) {
			// If no depth was specified, try for a 32-bit mode first, then a 16-bit mode,
			// then fall back on whatever.
			if(depth == 0) {
				mode = vbe_mode_find(width, height, 32);
				if(mode) {
					layout = determine_vbe_mode_layout(mode);
				}
				if(!mode || layout == 0) {
					mode = vbe_mode_find(width, height, 16);
					if(mode) {
						layout = determine_vbe_mode_layout(mode);
					}
					if(!mode || layout == 0) {
						mode = vbe_mode_find(width, height, 0);
						if(mode) {
							layout = determine_vbe_mode_layout(mode);
						}
					}
				}
				if(!layout) {
					boot_error("Unable to find supported %ix%i VBE mode.",
						   width, height);
				}
			} else {
				mode = vbe_mode_find(width, height, depth);
				if(mode) {
					layout = determine_vbe_mode_layout(mode);
				}
				if(!layout) {
					boot_error("Unable to find supported %ix%ix%i VBE mode.",
						   width, height, depth);
				}
			}
		} else {
			mode = default_vbe_mode;
			if(mode) {
				layout = determine_vbe_mode_layout(mode);
			}
		}
	} else {
		mode = default_vbe_mode;
		if(mode) {
			layout = determine_vbe_mode_layout(mode);
		}
	}

	if(!mode || !layout) {
		boot_error("Unable to find supported VBE mode.");
	}

	dprintf("mezzanine: Using %ix%i video mode, layout %i, pitch %i, fb at %08x\n",
		mode->info.x_resolution, mode->info.y_resolution, layout, mode->info.bytes_per_scan_line, mode->info.phys_base_ptr);
	boot_info->framebuffer_physical_address = fixnum(mode->info.phys_base_ptr);
	boot_info->framebuffer_width = fixnum(mode->info.x_resolution);
	boot_info->framebuffer_pitch = fixnum(mode->info.bytes_per_scan_line);
	boot_info->framebuffer_height = fixnum(mode->info.y_resolution);
	boot_info->framebuffer_layout = fixnum(layout);
	vbe_mode_set(mode);
}

/** Load the operating system. */
static __noreturn void mezzanine_loader_load(void) {
	mezzanine_loader_t *loader = current_environ->data;
	mmu_context_t *mmu = mmu_context_create(TARGET_TYPE_64BIT, PHYS_MEMORY_PAGETABLES);
	mmu_context_t *transition = mmu_context_create(TARGET_TYPE_64BIT, PHYS_MEMORY_INTERNAL);

	generate_pmap(mmu);

	for(uint32_t i = 0; i < loader->header.n_extents; ++i) {
		// Each extent must be 4k (block) aligned on-disk and
		// 4k (page) aligned in memory.
		dprintf("mezzanine: extent % 2" PRIu32 " %016" PRIx64 " %016" PRIx64 " %08" PRIx64 " %08" PRIx64 " %04" PRIx64 "\n",
			i, loader->header.extents[i].store_base, loader->header.extents[i].virtual_base,
			loader->header.extents[i].size, loader->header.extents[i].bump,
			loader->header.extents[i].flags);
		if(loader->header.extents[i].store_base % 4096 ||
		   loader->header.extents[i].virtual_base % PAGE_SIZE ||
		   loader->header.extents[i].size % PAGE_SIZE) {
			boot_error("Extent %" PRIu32 " is misaligned", i);
		}

		// Skip non-wired extents.
		if((loader->header.extents[i].flags & MEZZANINE_EXTENT_WIRED) == 0) {
			continue;
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

		// Initialize each region.
		// Stacks grow downwards, other regions grow upwards.
		if((loader->header.extents[i].flags & MEZZANINE_EXTENT_TYPE_MASK) == MEZZANINE_EXTENT_TYPE_STACK) {
			// Stacks do not need to be zero-initialized.
			if(!disk_read(loader->disk,
				      (void *)P2V(phys_addr + ROUND_DOWN(loader->header.extents[i].bump, 0x1000)),
				      loader->header.extents[i].size - ROUND_DOWN(loader->header.extents[i].bump, 0x1000),
				      loader->header.extents[i].store_base + ROUND_DOWN(loader->header.extents[i].bump, 0x1000))) {
				boot_error("Could not read extent %" PRIu32, i);
			}
		} else {
			if(!disk_read(loader->disk,
				      (void *)P2V(phys_addr),
				      ROUND_UP(loader->header.extents[i].bump, 0x1000),
				      loader->header.extents[i].store_base)) {
				boot_error("Could not read extent %" PRIu32, i);
			}
			memset((void*)P2V(phys_addr + loader->header.extents[i].bump),
			       0,
			       loader->header.extents[i].size - loader->header.extents[i].bump);
		}

		// Map this entry in.
		mmu_map(mmu, loader->header.extents[i].virtual_base, phys_addr,
			loader->header.extents[i].size);
		if((loader->header.extents[i].flags & MEZZANINE_EXTENT_TYPE_MASK) == MEZZANINE_EXTENT_TYPE_STACK) {
			mmu_map(mmu, loader->header.extents[i].virtual_base | MEZZANINE_STACK_MARK_BIT, phys_addr,
				loader->header.extents[i].size);
		}

		// Write block numbers to page info structs.
		for(phys_ptr_t i = 0; i < loader->header.extents[i].size; i += PAGE_SIZE) {
			set_page_info_bin(mmu, phys_addr + i, fixnum(loader->header.extents[i].virtual_base + i));
		}
	}

	loader_preboot();

	// Allocate the boot info page.
	phys_ptr_t boot_info_page;
	phys_memory_alloc(PAGE_SIZE, // size
			  0x1000, // alignment
			  0, 0, // min/max address
			  PHYS_MEMORY_ALLOCATED, // type
			  0, // flags
			  &boot_info_page);
	mezzanine_boot_information_t *boot_info = (mezzanine_boot_information_t *)P2V(boot_info_page);

	memcpy(boot_info->uuid, loader->header.uuid, 16);

	set_video_mode(boot_info);

	// Initialize buddy bins.
	for(int i = 0; i < mezzanine_n_buddy_bins; ++i) {
		boot_info->buddy_bin[i].first_page = loader->header.nil;
		boot_info->buddy_bin[i].count = fixnum(0);
	}

	// Generate the page tables used for transitioning from identity mapping
	// to the final page tables.
	// The loader must be identity mapped, and mapped in the physical region.
	ptr_t loader_start = ROUND_DOWN((ptr_t)__start, PAGE_SIZE);
	ptr_t loader_size = ROUND_UP((ptr_t)__end - (ptr_t)__start, PAGE_SIZE);
	mmu_map(transition, loader_start, loader_start, loader_size);
	mmu_map(transition, mezzanine_physical_map_address + loader_start, loader_start, loader_size);

	/* Reclaim all memory used internally. */
	memory_finalize();

	// For each free kboot memory region, add pages to the buddy allocator.
	// Also avoid any memory below 1MB, it's weird.
	// fixme: do this in a less stupid way (whole power-of-two chunks at a time, not single pages).
	LIST_FOREACH(&memory_ranges, iter) {
		memory_range_t *range = list_entry(iter, memory_range_t, header);

		for(phys_ptr_t i = 0; i < range->size; i += PAGE_SIZE) {
			if(range->type == PHYS_MEMORY_FREE && range->start + i > 1024 * 1024) {
				buddy_free_page(mmu, boot_info, loader->header.nil, range->start + i);
			}
		}
	}

	dprintf("mezzanine: Starting system...\n");
	mezzanine_arch_enter(transition->cr3,
			     mmu->cr3,
			     loader->header.entry_fref,
			     loader->header.initial_process,
			     fixnum(mezzanine_physical_map_address + boot_info_page));
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
		dprintf("mezzanine: extent % 2" PRIu32 " %016" PRIx64 " %016" PRIx64 " %08" PRIx64 " %08" PRIx64 " %04" PRIx64 "\n",
			i, data->header.extents[i].store_base, data->header.extents[i].virtual_base,
			data->header.extents[i].size, data->header.extents[i].bump,
			data->header.extents[i].flags);
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