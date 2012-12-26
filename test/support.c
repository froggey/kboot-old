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

#include <kboot.h>
#include <loader.h>

extern void console_putc(char ch);
extern void log_init(kboot_tag_t *tags);

/** KBoot log buffer. */
static kboot_log_t *kboot_log = NULL;
static size_t kboot_log_size = 0;

/** Get the length of a string.
 * @param str		Pointer to the string.
 * @return		Length of the string. */
size_t strlen(const char *str) {
	size_t ret;

	for(ret = 0; *str; str++, ret++) {}
	return ret;
}

/** Get length of a string with limit.
 * @param str		Pointer to the string.
 * @param count		Maximum length of the string.
 * @return		Length of the string. */
size_t strnlen(const char *str, size_t count) {
	size_t ret;

	for(ret = 0; *str && ret < count; str++, ret++) {}
	return ret;
}

/** Macro to implement strtoul() and strtoull(). */
#define __strtoux(type, cp, endp, base)		\
	__extension__ \
	({ \
		type result = 0, value; \
		if(!base) { \
			if(*cp == '0') { \
				if((tolower(*(++cp)) == 'x') && isxdigit(cp[1])) { \
					cp++; \
					base = 16; \
				} else { \
					base = 8; \
				} \
			} else { \
				base = 10; \
			} \
		} else if(base == 16) { \
			if(cp[0] == '0' && tolower(cp[1]) == 'x') \
				cp += 2; \
		} \
		\
		while(isxdigit(*cp) && (value = isdigit(*cp) \
			? *cp - '0' : tolower(*cp) - 'a' + 10) < base) \
		{ \
			result = result * base + value; \
			cp++; \
		} \
		\
		if(endp) \
			*endp = (char *)cp; \
		result; \
	})

/**
 * Convert a string to an unsigned long.
 *
 * Converts a string to an unsigned long using the specified number base.
 *
 * @param cp		The start of the string.
 * @param endp		Pointer to the end of the parsed string placed here.
 * @param base		The number base to use (if zero will guess).
 *
 * @return		Converted value.
 */
unsigned long strtoul(const char *cp, char **endp, unsigned int base) {
	return __strtoux(unsigned long, cp, endp, base);
}

/**
 * Convert a string to a signed long.
 *
 * Converts a string to an signed long using the specified number base.
 *
 * @param cp		The start of the string.
 * @param endp		Pointer to the end of the parsed string placed here.
 * @param base		The number base to use.
 *
 * @return		Converted value.
 */
long strtol(const char *cp, char **endp, unsigned int base) {
	if(*cp == '-')
		return -strtoul(cp + 1, endp, base);

	return strtoul(cp, endp, base);
}

/**
 * Convert a string to an unsigned long long.
 *
 * Converts a string to an unsigned long long using the specified number base.
 *
 * @param cp		The start of the string.
 * @param endp		Pointer to the end of the parsed string placed here.
 * @param base		The number base to use.
 *
 * @return		Converted value.
 */
unsigned long long strtoull(const char *cp, char **endp, unsigned int base) {
	return __strtoux(unsigned long long, cp, endp, base);
}

/**
 * Convert a string to an signed long long.
 *
 * Converts a string to an signed long long using the specified number base.
 *
 * @param cp		The start of the string.
 * @param endp		Pointer to the end of the parsed string placed here.
 * @param base		The number base to use.
 *
 * @return		Converted value.
 */
long long strtoll(const char *cp, char **endp, unsigned int base) {
	if(*cp == '-')
		return -strtoull(cp + 1, endp, base);

	return strtoull(cp, endp, base);
}

/** Helper for kvprintf().
 * @param ch		Character to display.
 * @param data		Console to use.
 * @param total		Pointer to total character count. */
static void kvprintf_helper(char ch, void *data, int *total) {
	console_putc(ch);

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
