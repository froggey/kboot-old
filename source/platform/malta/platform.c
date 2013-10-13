/*
 * Copyright (C) 2013 Alex Smith
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
 * @brief		MIPS Malta platform startup code.
 */

#include <lib/utility.h>

#include <malta/console.h>

#include <loader.h>
#include <memory.h>

/** Main function of the Malta loader.
 * @param argc		Argument count.
 * @param argv		Argument array.
 * @param envp		Environment array.
 * @param memsize	Memory size. */
void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
	/* Set up console output. */
	console_init();

	/* Initialize the architecture. */
	arch_init();

	/* Initialize the memory manager. The low 1MB is mostly reserved, but
	 * the YAMON code is marked as internal so that it can be freed once we
	 * enter the kernel. */
	phys_memory_add(0x1000, 0xef000, PHYS_MEMORY_INTERNAL);
	phys_memory_add(0x100000, ROUND_DOWN(memsize, PAGE_SIZE) - 0x100000,
		PHYS_MEMORY_FREE);
	memory_init();

	while(true) {}
}
