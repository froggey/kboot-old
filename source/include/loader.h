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
 * @brief		OS loader type structure.
 */

#ifndef __LOADER_H
#define __LOADER_H

#include <config.h>

/** Structure defining an OS loader type. */
typedef struct loader_type {
	/** Load the operating system.
	 * @note		Should not return. */
	void (*load)(void) __noreturn;

#if CONFIG_KBOOT_UI
	/** Display a configuration menu. */
	void (*configure)(void);
#endif
} loader_type_t;

#endif /* __LOADER_H */
