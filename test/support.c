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
 * @brief		Test kernel support functions.
 */

#include <lib/ctype.h>
#include <lib/printf.h>
#include <lib/string.h>
#include <lib/utility.h>

#include <memory.h>

#include "test.h"

/** KBoot log buffer. */
static kboot_log_t *kboot_log = NULL;
static size_t kboot_log_size = 0;

/** Main console. */
console_t *main_console = NULL;

/** Debug console. */
console_t *debug_console = NULL;

/** Helper for kvprintf().
 * @param ch		Character to display.
 * @param data		Console to use.
 * @param total		Pointer to total character count. */
static void kvprintf_helper(char ch, void *data, int *total) {
	if(debug_console)
		debug_console->putch(ch);
	if(main_console)
		main_console->putch(ch);

	if(kboot_log) {
		kboot_log->buffer[(kboot_log->start + kboot_log->length) % kboot_log_size] = ch;
		if(kboot_log->length < kboot_log_size) {
			kboot_log->length++;
		} else {
			kboot_log->start = (kboot_log->start + 1) % kboot_log_size;
		}
	}

	*total = *total + 1;
}

/** Output a formatted message to the console.
 * @param fmt		Format string used to create the message.
 * @param args		Arguments to substitute into format.
 * @return		Number of characters printed. */
int kvprintf(const char *fmt, va_list args) {
	return do_printf(kvprintf_helper, NULL, fmt, args);
}

/** Output a formatted message to the console.
 * @param fmt		Format string used to create the message.
 * @param ...		Arguments to substitute into format.
 * @return		Number of characters printed. */
int kprintf(const char *fmt, ...) {
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = kvprintf(fmt, args);
	va_end(args);

	return ret;
}

/** Initialize the log.
 * @param tags		Tag list. */
void log_init(kboot_tag_t *tags) {
	kboot_tag_log_t *log;

	while(tags->type != KBOOT_TAG_NONE) {
		if(tags->type == KBOOT_TAG_LOG) {
			log = (kboot_tag_log_t *)tags;
			kboot_log = (kboot_log_t *)((ptr_t)log->log_virt);
			kboot_log_size = log->log_size - sizeof(kboot_log_t);
			break;
		}

		tags = (kboot_tag_t *)ROUND_UP((ptr_t)tags + tags->size, 8);
	}
}

/** Dummy kmalloc() function. */
void *kmalloc(size_t size) {
	return NULL;
}

