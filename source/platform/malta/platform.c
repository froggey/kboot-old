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

#include <mips/memory.h>

#include <lib/string.h>
#include <lib/utility.h>

#include <malta/console.h>

#include <loader.h>
#include <memory.h>
#include <tar.h>

/** Main function of the Malta loader.
 * @param argc		Argument count.
 * @param argv		Argument array.
 * @param envp		Environment array.
 * @param memsize	Memory size. */
void platform_init(int argc, char **argv, char **envp, unsigned memsize) {
	ptr_t initrd_start = 0;
	size_t initrd_size = 0;
	phys_ptr_t start, end;
	char *str;
	int i;

	/* Set up console output. */
	console_init();

	/* Initialize the architecture. */
	arch_init();

	/* The boot image is passed to us as an initrd. The initrd location and
	 * size are passed on the command line. */
	for(i = 1; i < argc; i++) {
		str = strstr(argv[i], "rd_start=");
		if(str)
			initrd_start = strtoull(str + 9, NULL, 0);
		str = strstr(argv[i], "rd_size=");
		if(str)
			initrd_size = strtoull(str + 8, NULL, 0);
	}

	/* Add usable memory ranges. The low 1MB is mostly reserved, but the
	 * YAMON code is marked as internal so that it can be freed once we
	 * enter the kernel. */
	phys_memory_add(0x1000, 0xef000, PHYS_MEMORY_INTERNAL);
	phys_memory_add(0x100000, ROUND_DOWN(memsize, PAGE_SIZE) - 0x100000,
		PHYS_MEMORY_FREE);

	if(initrd_start >= KSEG0 && initrd_size) {
		start = ROUND_DOWN(initrd_start, PAGE_SIZE);
		end = ROUND_UP(initrd_start + initrd_size, PAGE_SIZE);
		phys_memory_add(V2P(initrd_start), end - start, PHYS_MEMORY_INTERNAL);
	}

	/* Initalize the memory manager. */
	memory_init();

	/* Mount the boot image. */
	if(initrd_start >= KSEG0 && initrd_size)
		tar_mount((void *)initrd_start, initrd_size);

	/* Call the main function. */
	loader_main();
}
