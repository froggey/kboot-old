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
 * @brief		BCM2835 UART console implementation.
 *
 * The BCM2835 has 2 UARTs, a mini UART (UART1) and a PL011 (UART0). Though
 * Broadcom says the mini UART is intended to be used as a console, everything
 * I've seen uses the full PL011. Both use the same RXD/TXD GPIO pins, which
 * one the pins are connected to is determined by which alternate function is
 * set for the GPIO pins (mini UART = ALT5, PL011 = ALT0).
 *
 * Reference:
 *  - PrimeCell UART (PL011) Technical Reference Manual
 *    http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/index.html
 *
 * @todo		Add a generic PL011 implementation, there'll be other
 *			boards that use a PL011 so we don't want to duplicate
 *			code for all of them.
 */

#include <bcm2835/bcm2835.h>
#include <bcm2835/gpio.h>
#include <bcm2835/uart.h>

#include <console.h>
#include <loader.h>

/** Baud rate to use. */
#define BAUD_RATE	115200

/** Mapping of the UART registers. */
static volatile uint32_t *uart_mapping = (volatile uint32_t *)BCM2835_UART0_BASE;

/** Write a character to the UART console.
 * @param ch		Character to write. */
static void uart_console_putch(char ch) {
	/* Wait until the transmit FIFO has space. */
	while(uart_mapping[PL011_REG_FR] & PL011_FR_TXFF);

	uart_mapping[PL011_REG_DR] = ch;
}

/** UART console. */
static console_t uart_console = {
	.putch = uart_console_putch,
};

/** Initialise the UART console. */
void uart_init(void) {
	uint32_t divider, fraction;

	/* Disable the UART while we configure it. */
	uart_mapping[PL011_REG_CR] = 0;

	/* Configure the TXD and RXD pins to point to the PL011 UART (ALT0). */
	gpio_select_function(14, GPIO_FUNC_ALT0);
	gpio_select_function(15, GPIO_FUNC_ALT0);

	/* Calculate the baud rate divisor registers. See PL011 Reference
	 * Manual, page 3-10.
	 *  Baud Rate Divisor = UARTCLK / (16 * Baud Rate)
	 * This is split into an integer and a fractional part.
	 *  FBRD = Round((64 * (UARTCLK % (16 * Baud Rate))) / (16 * Baud Rate))
	 */
	divider = UART0_CLOCK / (16 * BAUD_RATE);
	fraction = (8 * (UART0_CLOCK % (16 * BAUD_RATE))) / BAUD_RATE;
	fraction = (fraction >> 1) + (fraction & 1);

	uart_mapping[PL011_REG_IBRD] = divider;
	uart_mapping[PL011_REG_FBRD] = fraction;

	/* Initialize the line control register (8N1, FIFOs enabled). Note that
	 * a write to the LCR is required for an IBRD/FBRD change to take
	 * effect. */
	uart_mapping[PL011_REG_LCRH] = PL011_LCRH_FEN | PL011_LCRH_WLEN8;

	/* Enable the UART. */
	uart_mapping[PL011_REG_CR] = PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE;

	/* Set the debug console. */
	debug_console = &uart_console;

	dprintf("Hello, World!\n");
	while(1);
}
