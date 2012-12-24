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

#ifndef __MEMORY_H
#define __MEMORY_H

#include <arch/page.h>

#include <lib/list.h>

/** Structure used to represent a physical memory range. */
typedef struct memory_range {
	list_t header;			/**< Link to range list. */

	phys_ptr_t start;		/**< Start of the range. */
	phys_ptr_t end;			/**< End of the range. */
	unsigned type;			/**< Type of the range. */
} memory_range_t;

/** Physical memory range types. */
#define PHYS_MEMORY_FREE	0
#define PHYS_MEMORY_ALLOCATED	1
#define PHYS_MEMORY_RECLAIMABLE 2
#define PHYS_MEMORY_INTERNAL	3

/** Flags for phys_memory_alloc(). */
#define PHYS_ALLOC_RECLAIM	(1<<0)	/**< Mark the allocated range as reclaimable. */
#define PHYS_ALLOC_CANFAIL	(1<<1)	/**< The allocation is allowed to fail. */
#define PHYS_ALLOC_HIGH		(1<<2)	/**< Allocate the highest possible address. */

extern void *kmalloc(size_t size);
extern void *krealloc(void *addr, size_t size);
extern void kfree(void *addr);

extern void phys_memory_add(phys_ptr_t start, phys_ptr_t end, unsigned type);
extern void phys_memory_protect(phys_ptr_t start, phys_ptr_t end);
extern bool phys_memory_alloc(phys_ptr_t size, size_t align, phys_ptr_t min_addr,
	phys_ptr_t max_addr, unsigned flags, phys_ptr_t *physp);

extern void platform_memory_detect(void);
extern void memory_init(void);
extern list_t *memory_finalize(void);

#endif /* __MEMORY_H */
