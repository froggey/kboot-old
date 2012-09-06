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
 * @brief		BCM2835 GPIO definitions.
 */

#ifndef __BCM2835_GPIO_H
#define __BCM2835_GPIO_H

/** GPIO register definitions. */
#define GPIO_REG_FSEL0		0		/**< Function Select 0. */
#define GPIO_REG_SET0		7		/**< Pin Output Set 0. */
#define GPIO_REG_CLR0		10		/**< Pin Output Clear 0. */

/** GPIO function definitions. */
#define GPIO_FUNC_INPUT		0x0		/**< Input. */
#define GPIO_FUNC_OUTPUT	0x1		/**< Output. */
#define GPIO_FUNC_ALT0		0x4		/**< Alternative Function 0. */
#define GPIO_FUNC_ALT1		0x5		/**< Alternative Function 1. */
#define GPIO_FUNC_ALT2		0x6		/**< Alternative Function 2. */
#define GPIO_FUNC_ALT3		0x7		/**< Alternative Function 3. */
#define GPIO_FUNC_ALT4		0x3		/**< Alternative Function 4. */
#define GPIO_FUNC_ALT5		0x2		/**< Alternative Function 5. */

/** Number of GPIO pins. */
#define GPIO_PIN_COUNT		54

extern void gpio_select_function(unsigned pin, unsigned func);
extern void gpio_set_pin(unsigned pin);
extern void gpio_clear_pin(unsigned pin);

extern void gpio_init(void);

#endif /* __BCM2835_GPIO_H */
