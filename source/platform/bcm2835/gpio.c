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
 * @brief		BCM2835 GPIO functions.
 */

#include <bcm2835/bcm2835.h>
#include <bcm2835/gpio.h>

#include <assert.h>

/** Mapping of the GPIO registers. */
static volatile uint32_t *gpio_mapping = (volatile uint32_t *)BCM2835_GPIO_BASE;

/** Select the function of a GPIO pin.
 * @param pin		Pin number to change.
 * @param func		Function to set. */
void gpio_select_function(unsigned pin, unsigned func) {
	/* 3 bits per pin, 10 pins per function register. */
	size_t reg = GPIO_REG_FSEL0 + (pin / 10);
	uint32_t val = gpio_mapping[reg];

	val &= ~(7 << ((pin % 10) * 3));
	val |= (func << ((pin % 10) * 3));

	gpio_mapping[reg] = val;
}

/** Set an output GPIO pin.
 * @param pin		Pin number to set. */
void gpio_set_pin(unsigned pin) {
	size_t reg = GPIO_REG_SET0 + (pin / 32);

	/* Writes of 0s are ignored, just need to write the bit we want to
	 * set. */
	gpio_mapping[reg] = 1 << (pin % 32);
}

/** Clear an output GPIO pin.
 * @param pin		Pin number to clear. */
void gpio_clear_pin(unsigned pin) {
	size_t reg = GPIO_REG_CLR0 + (pin / 32);

	/* Writes of 0s are ignored, just need to write the bit we want to
	 * clear. */
	gpio_mapping[reg] = 1 << (pin % 32);
}
