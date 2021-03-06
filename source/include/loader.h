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
 * @brief		Core definitions.
 */

#ifndef __LOADER_H
#define __LOADER_H

#include <arch/loader.h>

#include <platform/loader.h>

#include <types.h>

/**
 * Offset to apply to a physical address to get a virtual address.
 *
 * To handle platforms where the loader runs from the virtual address space
 * and physical memory is not identity mapped, this value is added on to any
 * physical address used to obtain a virtual address that maps it. If it is
 * not specified by the architecture, it is assumed that physical addresses
 * can be used directly without modification.
 */
#ifndef LOADER_VIRT_OFFSET
# define LOADER_VIRT_OFFSET	0
#endif

/**
 * Highest physical address accessible to the loader.
 *
 * Specifies the highest physical address which the loader can access. If this
 * is not specified by the architecture, it is assumed that the loader can
 * access the low 4GB of the physical address space.
 */
#ifndef LOADER_PHYS_MAX
# define LOADER_PHYS_MAX	0xffffffff
#endif

/** Convert a virtual address to a physical address. */
#define V2P(a)		((phys_ptr_t)((a) - LOADER_VIRT_OFFSET))

/** Convert a physical address to a virtual address. */
#define P2V(a)		((ptr_t)((a) + LOADER_VIRT_OFFSET))

struct ui_window;

/** Structure defining an OS loader type. */
typedef struct loader_type {
	/** Load the operating system.
	 * @note		Should not return. */
	void (*load)(void) __noreturn;

	#if CONFIG_KBOOT_UI
	/** Return a window for configuring the OS.
	 * @return		Pointer to configuration window. */
	struct ui_window *(*configure)(void);
	#endif
} loader_type_t;

/** Builtin object definition structure. */
typedef struct builtin {
	/** Type of the builtin. */
	enum {
		BUILTIN_TYPE_COMMAND,
		BUILTIN_TYPE_FS,
		BUILTIN_TYPE_PARTITION_MAP,
	} type;

	/** Pointer to object. */
	void *object;
} builtin_t;

/** Type of a hook function to call before booting an OS. */
typedef void (*preboot_hook_t)(void);

extern char __start[], __end[];
extern builtin_t __builtins_start[], __builtins_end[];

/** Define a builtin object. */
#define DEFINE_BUILTIN(type, object) \
	static builtin_t __builtin_##name __section(".builtins") __used = { \
		type, \
		&object \
	}

/** Iterate over builtin objects. */
#define BUILTIN_ITERATE(btype, otype, vname) \
	int __iter_##vname = 0; \
	for(otype *vname = (otype *)__builtins_start[0].object; \
			__iter_##vname < (__builtins_end - __builtins_start); \
			vname = (otype *)__builtins_start[++__iter_##vname].object) \
		if(__builtins_start[__iter_##vname].type == btype)

extern int kvprintf(const char *fmt, va_list args);
extern int kprintf(const char *fmt, ...) __printf(1, 2);
extern int dvprintf(const char *fmt, va_list args);
extern int dprintf(const char *fmt, ...) __printf(1, 2);

extern void backtrace(int (*printfn)(const char *fmt, ...));

extern void internal_error(const char *fmt, ...) __printf(1, 2) __noreturn;
extern void boot_error(const char *fmt, ...) __printf(1, 2) __noreturn;

extern void loader_register_preboot_hook(preboot_hook_t hook);
extern void loader_preboot(void);

extern void loader_main(void) __noreturn;

#endif /* __LOADER_H */
