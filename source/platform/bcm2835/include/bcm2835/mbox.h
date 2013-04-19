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
 *  - Mailbox Property Interface - Raspberry Pi Firmware Wiki
 *    https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

#ifndef __BCM2835_MBOX_H
#define __BCM2835_MBOX_H

#include <types.h>

/** Mailbox register definitions. */
#define MBOX_REG_READ			0	/**< Receive. */
#define MBOX_REG_STATUS			6	/**< Poll. */
#define MBOX_REG_WRITE			8	/**< Write. */

/** Mailbox status register bits. */
#define MBOX_STATUS_FULL		(1<<31)	/**< Mailbox Full. */
#define MBOX_STATUS_EMPTY		(1<<30)	/**< Mailbox Empty. */

/** Mailbox channel definitions.*/
#define MBOX_CHANNEL_FB			1	/**< Framebuffer. */
#define MBOX_CHANNEL_PROP		8	/**< Property tags (ARM to VC). */

/** Property message header. */
typedef struct prop_message_header {
	uint32_t size;				/**< Total buffer size. */
	uint32_t code;				/**< Request/response code. */
} prop_message_header_t;

/** Property message end tag. */
typedef struct prop_message_footer {
	uint32_t end;				/**< End tag (0). */
} prop_message_footer_t;

/** Initialize a property request message. */
#define PROP_MESSAGE_INIT(msg)	\
	do { \
		STATIC_ASSERT(__alignof__(msg) == 16); \
		(msg).header.size = sizeof(msg); \
		(msg).header.code = 0; \
		(msg).footer.end = 0; \
	} while(0)

/** Property message tag header. */
typedef struct prop_tag_header {
	uint32_t id;				/**< Tag identifier. */
	uint32_t buf_size;			/**< Total size of tag buffer. */
	uint32_t req_size;			/**< Size of request data. */
} prop_tag_header_t;

/** Initialize a property tag. */
#define PROP_TAG_INIT(tag, _id)		\
	do { \
		(tag).header.id = (_id); \
		(tag).header.buf_size = sizeof((tag)) - sizeof((tag).header); \
		(tag).header.req_size = sizeof((tag).req); \
	} while(0)

/** Property channel request status codes. */
#define PROP_STATUS_SUCCESS		0x80000000
#define PROP_STATUS_FAILURE		0x80000001

/** Property channel request tag definitions. */
#define PROP_TAG_ALLOCATE_BUFFER	0x00040001
#define PROP_TAG_GET_PHYSICAL_SIZE	0x00040003
#define PROP_TAG_SET_PHYSICAL_SIZE	0x00048003
#define PROP_TAG_GET_VIRTUAL_SIZE	0x00040004
#define PROP_TAG_SET_VIRTUAL_SIZE	0x00048004
#define PROP_TAG_GET_DEPTH		0x00040005
#define PROP_TAG_SET_DEPTH		0x00048005
#define PROP_TAG_GET_PIXEL_ORDER	0x00040006
#define PROP_TAG_SET_PIXEL_ORDER	0x00048006
#define PROP_TAG_GET_ALPHA_MODE		0x00040007
#define PROP_TAG_SET_ALPHA_MODE		0x00048007
#define PROP_TAG_GET_PITCH		0x00040008
#define PROP_TAG_GET_VIRTUAL_OFFSET	0x00040009
#define PROP_TAG_SET_VIRTUAL_OFFSET	0x00048009

/** Allocate buffer tag definition. */
typedef struct prop_allocate_buffer {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t alignment;	/**< Buffer alignment. */
		} req;
		struct {
			uint32_t address;	/**< Framebuffer address. */
			uint32_t size;		/**< Framebuffer size. */
		} resp;
	};
} prop_allocate_buffer_t;

/** Get physical/virtual size tag definition. */
typedef struct prop_get_size {
	prop_tag_header_t header;
	union {
		struct {} req;
		struct {
			uint32_t width;		/**< Display width. */
			uint32_t height;	/**< Display height. */
		} resp;
	};
} prop_get_size_t;

/** Set physical/virtual size tag definition. */
typedef struct prop_set_size {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t width;		/**< Display width. */
			uint32_t height;	/**< Display height. */
		} req;
		struct {
			uint32_t width;		/**< Display width. */
			uint32_t height;	/**< Display height. */
		} resp;
	};
} prop_set_size_t;

/** Set depth tag definition. */
typedef struct prop_set_depth {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t depth;		/**< Display depth. */
		} req;
		struct {
			uint32_t depth;		/**< Display depth. */
		} resp;
	};
} prop_set_depth_t;

/** Set pixel order tag definition. */
typedef struct prop_set_pixel_order {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t state;		/**< Pixel order. */
		} req;
		struct {
			uint32_t state;		/**< Pixel order. */
		} resp;
	};
} prop_set_pixel_order_t;

/** Set alpha mode tag definition. */
typedef struct prop_set_alpha_mode {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t state;		/**< Alpha mode. */
		} req;
		struct {
			uint32_t state;		/**< Alpha mode. */
		} resp;
	};
} prop_set_alpha_mode_t;

/** Get pitch tag definition. */
typedef struct prop_get_pitch {
	prop_tag_header_t header;
	union {
		struct {} req;
		struct {
			uint32_t pitch;		/**< Pitch. */
		} resp;
	};
} prop_get_pitch_t;

/** Set physical/virtual size tag definition. */
typedef struct prop_set_offset {
	prop_tag_header_t header;
	union {
		struct {
			uint32_t x;		/**< X offset in pixels. */
			uint32_t y;		/**< Y offset in pixels. */
		} req;
		struct {
			uint32_t x;		/**< X offset in pixels. */
			uint32_t y;		/**< Y offset in pixels. */
		} resp;
	};
} prop_set_offset_t;

extern uint32_t mbox_read(uint8_t channel);
extern void mbox_write(uint8_t channel, uint32_t data);

extern bool mbox_prop_request(void *buffer);

#endif /* __BCM2835_MBOX_H */
