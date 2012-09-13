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
 * @brief		Memory management functions.
 */

#include <lib/list.h>
#include <lib/string.h>
#include <lib/utility.h>

#include <assert.h>
#include <kboot.h>
#include <loader.h>
#include <memory.h>

/** Structure representing an area on the heap. */
typedef struct heap_chunk {
	list_t header;			/**< Link to chunk list. */
	unsigned long size;		/**< Size of chunk including struct (low bit == used). */
} heap_chunk_t;

/** Structure used to represent a physical memory range internally. */
typedef struct memory_range {
	list_t header;			/**< Link to range list. */
	kboot_tag_memory_t ka;		/**< Actual range structure. */
} memory_range_t;

/** Size of the heap (128KB). */
#define HEAP_SIZE		131072

extern char __start[], __end[], loader_stack[];

/** Statically allocated heap. */
static uint8_t heap[HEAP_SIZE] __aligned(PAGE_SIZE);
static LIST_DECLARE(heap_chunks);

/** List of physical memory ranges. */
static LIST_DECLARE(memory_ranges);

/** Get the size of a heap chunk.
 * @param chunk		Chunk to get size of.
 * @return		Size of chunk. */
static inline size_t heap_chunk_size(heap_chunk_t *chunk) {
	return (chunk->size & ~(1<<0));
}

/** Check whether a heap chunk is free.
 * @param chunk		Chunk to check.
 * @return		Whether chunk is free. */
static inline bool heap_chunk_free(heap_chunk_t *chunk) {
	return !(chunk->size & (1<<0));
}

/** Allocate memory from the heap.
 * @note		An internal error will be raised if heap is full.
 * @param size		Size of allocation to make.
 * @return		Address of allocation. */
void *kmalloc(size_t size) {
	heap_chunk_t *chunk = NULL, *new;
	size_t total;

	if(size == 0)
		internal_error("Zero-sized allocation!");

	/* Align all allocations to 8 bytes. */
	size = ROUND_UP(size, 8);
	total = size + sizeof(heap_chunk_t);

	/* Create the initial free segment if necessary. */
	if(list_empty(&heap_chunks)) {
		chunk = (heap_chunk_t *)heap;
		chunk->size = HEAP_SIZE;
		list_init(&chunk->header);
		list_append(&heap_chunks, &chunk->header);
	} else {
		/* Search for a free chunk. */
		LIST_FOREACH(&heap_chunks, iter) {
			chunk = list_entry(iter, heap_chunk_t, header);
			if(heap_chunk_free(chunk) && chunk->size >= total) {
				break;
			} else {
				chunk = NULL;
			}
		}

		if(!chunk)
			internal_error("Exhausted heap space (want %zu bytes)", size);
	}

	/* Resize the segment if it is too big. There must be space for a
	 * second chunk header afterwards. */
	if(chunk->size >= (total + sizeof(heap_chunk_t))) {
		new = (heap_chunk_t *)((char *)chunk + total);
		new->size = chunk->size - total;
		list_init(&new->header);
		list_add_after(&chunk->header, &new->header);
		chunk->size = total;
	}

	chunk->size |= (1<<0);
	return ((char *)chunk + sizeof(heap_chunk_t));
}

/** Resize a memory allocation.
 * @param addr		Address of old allocation.
 * @param size		New size of allocation.
 * @return		Address of new allocation, or NULL if size is 0. */
void *krealloc(void *addr, size_t size) {
	heap_chunk_t *chunk;
	void *new;

	if(size == 0) {
		kfree(addr);
		return NULL;
	} else {
		new = kmalloc(size);
		if(addr) {
			chunk = (heap_chunk_t *)((char *)addr - sizeof(heap_chunk_t));
			memcpy(new, addr, MIN(heap_chunk_size(chunk) - sizeof(heap_chunk_t), size));
			kfree(addr);
		}
		return new;
	}
}

/** Free memory allocated with kfree().
 * @param addr		Address of allocation. */
void kfree(void *addr) {
	heap_chunk_t *chunk, *adj;

	if(!addr)
		return;

	/* Get the chunk and free it. */
	chunk = (heap_chunk_t *)((char *)addr - sizeof(heap_chunk_t));
	if(heap_chunk_free(chunk))
		internal_error("Double free on address %p", addr);
	chunk->size &= ~(1<<0);

	/* Coalesce adjacent free segments. */
	if(chunk->header.next != &heap_chunks) {
		adj = list_entry(chunk->header.next, heap_chunk_t, header);
		if(heap_chunk_free(adj)) {
			assert(adj == (heap_chunk_t *)((char *)chunk + chunk->size));
			chunk->size += adj->size;
			list_remove(&adj->header);
		}
	}
	if(chunk->header.prev != &heap_chunks) {
		adj = list_entry(chunk->header.prev, heap_chunk_t, header);
		if(heap_chunk_free(adj)) {
			assert(chunk == (heap_chunk_t *)((char *)adj + adj->size));
			adj->size += chunk->size;
			list_remove(&chunk->header);
		}
	}
}

/** Allocate a memory range structure.
 * @param start		Start address.
 * @param end		End address.
 * @param type		Type of range.
 * @return		Pointer to range structure. */
static memory_range_t *memory_range_alloc(phys_ptr_t start, phys_ptr_t end, int type) {
	memory_range_t *range = kmalloc(sizeof(memory_range_t));
	list_init(&range->header);
	range->ka.start = start;
	range->ka.end = end;
	range->ka.type = type;
	return range;
}

/** Merge adjacent ranges.
 * @param range		Range to merge. */
static void memory_range_merge(memory_range_t *range) {
	memory_range_t *other;

	if(memory_ranges.next != &range->header) {
		other = list_entry(range->header.prev, memory_range_t, header);
		if(other->ka.end == range->ka.start && other->ka.type == range->ka.type) {
			range->ka.start = other->ka.start;
			list_remove(&other->header);
			kfree(other);
		}
	}
	if(memory_ranges.prev != &range->header) {
		other = list_entry(range->header.next, memory_range_t, header);
		if(other->ka.start == range->ka.end && other->ka.type == range->ka.type) {
			range->ka.end = other->ka.end;
			list_remove(&other->header);
			kfree(other);
		}
	}
}

/** Dump a list of physical memory ranges. */
static void phys_memory_dump(void) {
	memory_range_t *range;

	LIST_FOREACH(&memory_ranges, iter) {
		range = list_entry(iter, memory_range_t, header);

		dprintf(" 0x%016" PRIxPHYS "-0x%016" PRIxPHYS ": ", range->ka.start, range->ka.end);
		switch(range->ka.type) {
		case PHYS_MEMORY_FREE:
			dprintf("Free\n");
			break;
		case PHYS_MEMORY_ALLOCATED:
			dprintf("Allocated\n");
			break;
		case PHYS_MEMORY_RECLAIMABLE:
			dprintf("Reclaimable\n");
			break;
		case PHYS_MEMORY_RESERVED:
			dprintf("Reserved\n");
			break;
		case PHYS_MEMORY_UNUSABLE:
			dprintf("Unusable\n");
			break;
		case PHYS_MEMORY_INTERNAL:
			dprintf("Internal\n");
			break;
		default:
			dprintf("???\n");
			break;
		}
	}
}

/** Add a range of physical memory.
 * @param start		Start of the range (must be page-aligned).
 * @param end		End of the range (must be page-aligned).
 * @param type		Type of the range. */
static void phys_memory_add_internal(phys_ptr_t start, phys_ptr_t end, int type) {
	memory_range_t *range, *other, *split;

	assert(!(start % PAGE_SIZE));
	assert(!(end % PAGE_SIZE));
	assert(end > start);

	range = memory_range_alloc(start, end, type);

	/* Try to find where to insert the region in the list. */
	LIST_FOREACH(&memory_ranges, iter) {
		other = list_entry(iter, memory_range_t, header);
		if(start <= other->ka.start) {
			list_add_before(&other->header, &range->header);
			break;
		}
	}

	/* If the range has not been added, add it now. */
	if(list_empty(&range->header))
		list_append(&memory_ranges, &range->header);

	/* Check if the new range has overlapped part of the previous range. */
	if(memory_ranges.next != &range->header) {
		other = list_entry(range->header.prev, memory_range_t, header);
		if(range->ka.start < other->ka.end) {
			if(other->ka.end > range->ka.end) {
				/* Must split the range. */
				split = memory_range_alloc(range->ka.end, other->ka.end, other->ka.type);
				list_add_after(&range->header, &split->header);
			}
			other->ka.end = range->ka.start;
		}
	}

	/* Swallow up any following ranges that the new range overlaps. */
	LIST_FOREACH_SAFE(&range->header, iter) {
		if(iter == &memory_ranges)
			break;

		other = list_entry(iter, memory_range_t, header);
		if(other->ka.start >= range->ka.end) {
			break;
		} else if(other->ka.end > range->ka.end) {
			/* Resize the range and finish. */
			other->ka.start = range->ka.end;
			break;
		} else {
			/* Completely remove the range. */
			list_remove(&other->header);
		}
	}

	/* Finally, merge the region with adjacent ranges of the same type. */
	memory_range_merge(range);
}

/** Add a range of physical memory.
 * @param start		Start of the range (must be page-aligned).
 * @param end		End of the range (must be page-aligned).
 * @param type		Type of the range. */
void phys_memory_add(phys_ptr_t start, phys_ptr_t end, int type) {
	phys_memory_add_internal(start, end, type);
	dprintf("memory: added range 0x%" PRIxPHYS "-0x%" PRIxPHYS " (type: %d)\n",
		start, end, type);
}

/** Prevent allocations from being made from a range of physical memory.
 * @param start		Start of the range.
 * @param end		End of the range. */
void phys_memory_protect(phys_ptr_t start, phys_ptr_t end) {
	memory_range_t *range;

	start = ROUND_DOWN(start, PAGE_SIZE);
	end = ROUND_UP(end, PAGE_SIZE);

	LIST_FOREACH_SAFE(&memory_ranges, iter) {
		range = list_entry(iter, memory_range_t, header);

		if(range->ka.type != PHYS_MEMORY_FREE) {
			continue;
		} else if(start >= range->ka.start && start < range->ka.end) {
			phys_memory_add_internal(start, MIN(end, range->ka.end), PHYS_MEMORY_INTERNAL);
		} else if(end > range->ka.start && end <= range->ka.end) {
			phys_memory_add_internal(MAX(start, range->ka.start), end, PHYS_MEMORY_INTERNAL);
		}
	}
}

/** Check whether a range can satisfy an allocation
 * @param range		Range to check.
 * @param size		Size of the allocation.
 * @param align		Alignment of the allocation.
 * @param min_addr	Minimum address for the start of the allocated range.
 * @param max_addr	Maximum address of the end of the allocated range.
 * @param flags		Behaviour flags.
 * @param physp		Where to store address for allocation.
 * @return		Whether the range can satisfy the allocation. */
static bool is_suitable_range(memory_range_t *range, phys_ptr_t size, size_t align,
	phys_ptr_t min_addr, phys_ptr_t max_addr, unsigned flags,
	phys_ptr_t *physp)
{
	phys_ptr_t start, match_start, match_end;

	if(range->ka.type != PHYS_MEMORY_FREE)
		return false;

	/* Check if this range contains addresses in the requested range. */
	match_start = MAX(min_addr, range->ka.start);
	match_end = MIN(max_addr - 1, range->ka.end - 1);
	if(match_end <= match_start)
		return false;

	/* Align the base address and check that the range fits. */
	if(flags & PHYS_ALLOC_HIGH) {
		start = ROUND_DOWN((match_end - size) + 1, align);
		if(start < match_start)
			return false;
	} else {
		start = ROUND_UP(match_start, align);
		if((start + size - 1) > match_end)
			return false;
	}

	*physp = start;
	return true;
}

/**
 * Allocate a range of physical memory.
 *
 * Allocates a range of physical memory satisfying the specified constraints.
 * Unless PHYS_ALLOC_CANFAIL is specified, a boot error will be raised if the
 * allocation fails.
 *
 * @param size		Size of the range (multiple of PAGE_SIZE).
 * @param align		Alignment of the range (power of 2, at least PAGE_SIZE).
 * @param min_addr	Minimum address for the start of the allocated range.
 * @param max_addr	Maximum address of the end of the allocated range.
 * @param flags		Behaviour flags.
 * @param physp		Where to store address of allocation.
 *
 * @return		Whether successfully allocated (always true unless
 *			PHYS_ALLOC_CANFAIL specified).
 */
bool phys_memory_alloc(phys_ptr_t size, size_t align, phys_ptr_t min_addr, phys_ptr_t max_addr,
	unsigned flags, phys_ptr_t *physp)
{
	memory_range_t *range;
	phys_ptr_t start;

	if(!align)
		align = PAGE_SIZE;

	assert(!(size % PAGE_SIZE));
	assert(!(min_addr % align));
	assert(((max_addr - 1) - min_addr) >= (size - 1));

	/* Find a free range that is large enough to hold the new range. */
	if(flags & PHYS_ALLOC_HIGH) {
		LIST_FOREACH_R(&memory_ranges, iter) {
			range = list_entry(iter, memory_range_t, header);
			if(is_suitable_range(range, size, align, min_addr, max_addr, flags, &start))
				break;

			range = NULL;
		}
	} else {
		LIST_FOREACH(&memory_ranges, iter) {
			range = list_entry(iter, memory_range_t, header);
			if(is_suitable_range(range, size, align, min_addr, max_addr, flags, &start))
				break;

			range = NULL;
		}
	}

	if(!range) {
		if(!(flags & PHYS_ALLOC_CANFAIL))
			boot_error("You do not have enough memory available");

		return false;
	}

	/* Insert a new range over the top of the allocation. */
	phys_memory_add_internal(start, start + size, (flags & PHYS_ALLOC_RECLAIM)
		? PHYS_MEMORY_RECLAIMABLE : PHYS_MEMORY_ALLOCATED);

	dprintf("memory: allocated 0x%" PRIxPHYS "-0x%" PRIxPHYS " (align: 0x%zx, flags: 0x%x)\n",
		start, start + size, align, flags);
	*physp = start;
	return true;
}

/** Initialise the memory manager. */
void memory_init(void) {
	/* Detect memory ranges. */
	platform_memory_detect();

	/* Mark the bootloader itself as internal so that it gets reclaimed
	 * before entering the kernel, and mark the heap as reclaimable so the
	 * kernel can get rid of it once it has finished with the arguments. */
	phys_memory_add(ROUND_DOWN((phys_ptr_t)((ptr_t)__start), PAGE_SIZE),
		(phys_ptr_t)((ptr_t)__end), PHYS_MEMORY_INTERNAL);
	phys_memory_add((ptr_t)heap, (ptr_t)heap + HEAP_SIZE, PHYS_MEMORY_RECLAIMABLE);

	/* Mark the stack as reclaimable. */
	phys_memory_add((ptr_t)loader_stack, (ptr_t)loader_stack + PAGE_SIZE, PHYS_MEMORY_RECLAIMABLE);

	dprintf("memory: initial memory map:\n");
	phys_memory_dump();
}

/** Finalise the memory map.
 * @note		Only needs to be called when booting a KBoot kernel.
 * @return		Physical address of the first memory range tag. */
phys_ptr_t memory_finalise(void) {
	memory_range_t *range, *next;
	uint32_t i = 0;

	/* Reclaim all internal memory ranges. */
	LIST_FOREACH(&memory_ranges, iter) {
		range = list_entry(iter, memory_range_t, header);
		if(range->ka.type == PHYS_MEMORY_INTERNAL) {
			range->ka.type = PHYS_MEMORY_FREE;
			memory_range_merge(range);
		}
	}

	/* Dump the memory map to the debug console. */
	dprintf("memory: final memory map:\n");
	phys_memory_dump();

	/* Set up the tag headers in the range structures. */
	LIST_FOREACH(&memory_ranges, iter) {
		range = list_entry(iter, memory_range_t, header);

		range->ka.header.type = KBOOT_TAG_MEMORY;
		range->ka.header.size = sizeof(range->ka);
		if(range->header.next != &memory_ranges) {
			next = list_entry(range->header.next, memory_range_t, header);
			range->ka.header.next = (ptr_t)&next->ka;
		} else {
			range->ka.header.next = 0;
		}

		i++;
	}

	range = list_entry(memory_ranges.next, memory_range_t, header);
	return (ptr_t)&range->ka;
}
