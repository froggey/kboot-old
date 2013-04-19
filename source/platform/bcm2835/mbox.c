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
 * @brief		BCM2835 mailbox functions.
 *
 * Reference:
 *  - Accessing Mailboxes - Raspberry Pi Firmware Wiki
 *    https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
 *  - RPi Framebuffer - eLinux.org
 *    http://elinux.org/RPi_Framebuffer#Mailbox_operations
 */

#include <arm/cpu.h>

#include <bcm2835/bcm2835.h>
#include <bcm2835/mbox.h>

#include <assert.h>

/** Mapping of the mailbox registers. */
static volatile uint32_t *mbox_mapping = (volatile uint32_t *)BCM2835_MBOX0_BASE;

/** Read a value from the mailbox.
 * @param channel	Mailbox channel to read from.
 * @return		Value read from the mailbox. */
uint32_t mbox_read(uint8_t channel) {
	uint32_t value;

	assert(!(channel & 0xF0));

	while(true) {
		while(true) {
			arm_dmb();
			if(!(mbox_mapping[MBOX_REG_STATUS] & MBOX_STATUS_EMPTY))
				break;
		}

		value = mbox_mapping[MBOX_REG_READ];
		arm_dmb();

		if((value & 0xF) != channel)
			continue;

		return (value & 0xFFFFFFF0);
	}
}

/** Write a value to the mailbox.
 * @param channel	Mailbox channel to write.
 * @param data		Data to write. */
void mbox_write(uint8_t channel, uint32_t data) {
	uint32_t value;

	assert(!(channel & 0xF0));
	assert(!(data & 0xF));

	/* Drain any pending responses. */
	while(true) {
		arm_dmb();
		if(mbox_mapping[MBOX_REG_STATUS] & MBOX_STATUS_EMPTY)
			break;

		value = mbox_mapping[MBOX_REG_READ];
		arm_dmb();
	}

	while(true) {
		arm_dmb();
		if(!(mbox_mapping[MBOX_REG_STATUS] & MBOX_STATUS_FULL))
			break;
	}

	arm_dmb();
	mbox_mapping[MBOX_REG_WRITE] = data | (uint32_t)channel;
}

/** Send a request on the property channel and get the response.
 * @param buffer	Buffer containing property request.
 * @return		Whether the request was completed successfully. */
bool mbox_prop_request(void *buffer) {
	uint32_t val;

	assert(!((uint32_t)buffer & 0xF));

	mbox_write(MBOX_CHANNEL_PROP, (uint32_t)buffer);
	val = mbox_read(MBOX_CHANNEL_PROP);

	if(val != (uint32_t)buffer) {
		dprintf("mbox: request returned mismatching buffer address 0x%lx, "
			"should be 0x%lx\n", val, (uint32_t)buffer);
		return false;
	}

	val = ((volatile uint32_t *)buffer)[1];

	if(val != PROP_STATUS_SUCCESS) {
		dprintf("mbox: request failed, status: 0x%lx\n", val);
		return false;
	}

	return true;
}
