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
 */

#include <bcm2835/bcm2835.h>
#include <bcm2835/gpio.h>
#include <bcm2835/uart.h>

#include <pl011/pl011.h>

#include <loader.h>

/** Initialise the UART console. */
void uart_init(void) {
	/* Configure the TXD and RXD pins to point to the PL011 UART (ALT0). */
	gpio_select_function(14, GPIO_FUNC_ALT0);
	gpio_select_function(15, GPIO_FUNC_ALT0);

	pl011_init(BCM2835_UART0_BASE, UART0_CLOCK);
}
