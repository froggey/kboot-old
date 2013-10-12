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
 * @brief		Malta port I/O functions.
 *
 * Malta implements PC-style I/O ports, except the I/O ports are mapped into
 * the physical address space.
 */

#ifndef __MALTA_IO_H
#define __MALTA_IO_H

#include <mips/memory.h>

#include <malta/malta.h>

#include <types.h>

/** Calculate an I/O port address. */
#define IO_PORT_ADDR(port)	(KSEG1ADDR((ptr_t)MALTA_IO_PORT_BASE) + port)

/** Read 8 bits from a port.
 * @param port		Port to read from.
 * @return		Value read. */
static inline uint8_t in8(uint16_t port) {
	return *(volatile uint8_t *)IO_PORT_ADDR(port);
}

/** Write 8 bits to a port.
 * @param port		Port to write to.
 * @param data		Value to write. */
static inline void out8(uint16_t port, uint8_t data) {
	*(volatile uint8_t *)IO_PORT_ADDR(port) = data;
}

/** Read 16 bits from a port.
 * @param port		Port to read from.
 * @return		Value read. */
static inline uint16_t in16(uint16_t port) {
	return *(volatile uint16_t *)IO_PORT_ADDR(port);
}

/** Write 16 bits to a port.
 * @param port		Port to write to.
 * @param data		Value to write. */
static inline void out16(uint16_t port, uint16_t data) {
	*(volatile uint16_t *)IO_PORT_ADDR(port) = data;
}

/** Read 32 bits from a port.
 * @param port		Port to read from.
 * @return		Value read. */
static inline uint32_t in32(uint16_t port) {
	return *(volatile uint32_t *)IO_PORT_ADDR(port);
}

/** Write 32 bits to a port.
 * @param port		Port to write to.
 * @param data		Value to write. */
static inline void out32(uint16_t port, uint32_t data) {
	*(volatile uint32_t *)IO_PORT_ADDR(port) = data;
}

#endif /* __MALTA_IO_H */
