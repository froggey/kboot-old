/*
 * Copyright (C) 2012 Alex Smith
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
 * @brief		PL011 UART definitions.
 */

#ifndef __PL011_PL011_H
#define __PL011_PL011_H

#include <types.h>

/** PL011 UART port definitions. */
#define PL011_REG_DR		0	/**< Data Register. */
#define PL011_REG_RSR		1	/**< Receive Status Register. */
#define PL011_REG_ECR		1	/**< Error Clear Register. */
#define PL011_REG_FR		6	/**< Flag Register. */
#define PL011_REG_IBRD		9	/**< Integer Baud Rate Register. */
#define PL011_REG_FBRD		10	/**< Fractional Baud Rate Register. */
#define PL011_REG_LCRH		11	/**< Line Control Register. */
#define PL011_REG_CR		12	/**< Control Register. */
#define PL011_REG_IFLS		13	/**< Interrupt FIFO Level Select Register. */
#define PL011_REG_IMSC		14	/**< Interrupt Mask Set/Clear Register. */
#define PL011_REG_RIS		15	/**< Raw Interrupt Status Register. */
#define PL011_REG_MIS		16	/**< Masked Interrupt Status Register. */
#define PL011_REG_ICR		17	/**< Interrupt Clear Register. */
#define PL011_REG_DMACR		18	/**< DMA Control Register. */

/** PL011 flag register bits. */
#define PL011_FR_TXFF		(1<<5)	/**< Transmit FIFO full. */
#define PL011_FR_RXFE		(1<<4)	/**< Receive FIFO empty. */

/** PL011 line control register bits. */
#define PL011_LCRH_FEN		(1<<4)	/**< Enable FIFOs. */
#define PL011_LCRH_WLEN5	(0<<5)	/**< 5 data bits. */
#define PL011_LCRH_WLEN6	(1<<5)	/**< 6 data bits. */
#define PL011_LCRH_WLEN7	(2<<5)	/**< 7 data bits. */
#define PL011_LCRH_WLEN8	(3<<5)	/**< 8 data bits. */

/** PL011 control register bit definitions. */
#define PL011_CR_UARTEN		(1<<0)	/**< UART enable. */
#define PL011_CR_TXE		(1<<8)	/**< Transmit enable. */
#define PL011_CR_RXE		(1<<9)	/**< Receive enable. */

extern void pl011_init(ptr_t mapping, uint32_t clock_rate);

#endif /* __PL011_PL011_H */
