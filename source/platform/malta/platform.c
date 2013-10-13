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

	dprintf("Hello, World! argc = %d, argv = %p, envp = %p, memsize = 0x%x\n",
		argc, argv, envp, memsize);

	for(int i = 0; i < argc; i++) {
		dprintf(" argv[%d] = '%s'\n", i, argv[i]);
	}

	int i = 0;
	while(envp[i]) {
		dprintf(" envp[%d] = '%s'\n", i, envp[i]);
		i++;
	}

	while(true) {}
}
