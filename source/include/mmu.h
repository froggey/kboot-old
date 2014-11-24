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
 * @brief		MMU functions.
 */

#ifndef __MMU_H
#define __MMU_H

#include <target.h>

/** Type of an MMU context. */
typedef struct mmu_context mmu_context_t;

extern bool mmu_map(mmu_context_t *ctx, target_ptr_t virt, phys_ptr_t phys,
	target_size_t size);
extern bool mmu_alias(mmu_context_t *ctx, target_ptr_t target, target_ptr_t source,
	target_size_t size);

extern mmu_context_t *mmu_context_create(target_type_t target, unsigned phys_type);

extern void mmu_memset(mmu_context_t *ctx, target_ptr_t addr, uint8_t value, target_size_t size);
extern void mmu_memcpy_to(mmu_context_t *ctx, target_ptr_t addr, const void *source, target_size_t size);
extern void mmu_memcpy_from(mmu_context_t *ctx, void *dest, target_ptr_t addr, target_size_t size);

#endif /* __MMU_H */
