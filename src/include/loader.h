/*
 * Copyright (C) 2010-2011 Alex Smith
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
 * @brief		Loader type structure.
 */

#ifndef __LOADER_H
#define __LOADER_H

#include <arch/loader.h>

#include <platform/loader.h>

#include <config.h>
#include <fs.h>
#include <ui.h>

/** Structure defining a loader type. */
typedef struct loader_type {
	/** Load the operating system.
	 * @note		Should not return.
	 * @param env		Environment for the OS. */
	void (*load)(environ_t *env) __noreturn;

	/** Display a configuration menu.
	 * @param env		Environment for the OS. */
	void (*configure)(environ_t *env);
} loader_type_t;

extern loader_type_t *loader_type_get(environ_t *env);
extern void loader_type_set(environ_t *env, loader_type_t *type);
extern void *loader_data_get(environ_t *env);
extern void loader_data_set(environ_t *env, void *data);

extern void internal_error(const char *fmt, ...) __printf(1, 2) __noreturn;
extern void boot_error(const char *fmt, ...) __printf(1, 2) __noreturn;

#endif /* __LOADER_H */
