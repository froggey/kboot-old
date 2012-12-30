/*
 * Copyright (C) 2011 Alex Smith
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
 * @brief		Test kernel console functions.
 */

#include <arch/io.h>

#include <lib/string.h>
#include <lib/utility.h>

#include "../test.h"

/** VGA register definitions. */
#define VGA_CRTC_INDEX		0x3D4
#define VGA_CRTC_DATA		0x3D5

/** VGA attributes. */
#define VGA_ATTRIB		0x0F00

/** Puts a character into the VGA memory. */
#define VGA_WRITE_WORD(idx, ch)		\
	{ vga_mapping[idx] = ((uint16_t)ch | VGA_ATTRIB); }

/** Serial port to use. */
#define SERIAL_PORT		0x3F8

/** VGA details. */
static uint16_t *vga_mapping;
static uint16_t vga_cursor_x = 0;
static uint16_t vga_cursor_y = 0;
static uint16_t vga_cols = 80;
static uint16_t vga_lines = 25;

/** Write a character to the serial console.
 * @param ch		Character to write. */
static void serial_console_putch(char ch) {
	if(ch == '\n')
		serial_console_putch('\r');

	out8(SERIAL_PORT, ch);
	while(!(in8(SERIAL_PORT + 5) & 0x20));
}

/** Debug console. */
static console_t serial_console = {
	.putch = serial_console_putch,
};

/** Initialize the serial port. */
static inline void init_serial_console(void) {
	out8(SERIAL_PORT + 1, 0x00);  /* Disable all interrupts */
	out8(SERIAL_PORT + 3, 0x80);  /* Enable DLAB (set baud rate divisor) */
	out8(SERIAL_PORT + 0, 0x03);  /* Set divisor to 3 (lo byte) 38400 baud */
	out8(SERIAL_PORT + 1, 0x00);  /*                  (hi byte) */
	out8(SERIAL_PORT + 3, 0x03);  /* 8 bits, no parity, one stop bit */
	out8(SERIAL_PORT + 2, 0xC7);  /* Enable FIFO, clear them, with 14-byte threshold */
	out8(SERIAL_PORT + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */

	debug_console = &serial_console;
}

/** Scroll the VGA console. */
static void vga_console_scroll(void) {
	uint16_t i;

	memmove(vga_mapping, vga_mapping + vga_cols, (vga_lines - 1) * vga_cols * 2);

	for(i = 0; i < vga_cols; i++)
		VGA_WRITE_WORD(((vga_lines - 1) * vga_cols) + i, ' ');

	vga_cursor_y = vga_lines - 1;
}

/** Move the VGA cursor. */
static void vga_console_move_csr(void) {
	out8(VGA_CRTC_INDEX, 14);
	out8(VGA_CRTC_DATA, ((vga_cursor_y * vga_cols) + vga_cursor_x) >> 8);
	out8(VGA_CRTC_INDEX, 15);
	out8(VGA_CRTC_DATA, (vga_cursor_y * vga_cols) + vga_cursor_x);
}

/** Write a character to the VGA console.
 * @param ch		Character to write. */
static void vga_console_putch(char ch) {
	switch(ch) {
	case '\b':
		/* Backspace, move back one character if we can. */
		if(vga_cursor_x != 0) {
			vga_cursor_x--;
		} else {
			vga_cursor_x = vga_cols - 1;
			vga_cursor_y--;
		}
		break;
	case '\r':
		/* Carriage return, move to the start of the line. */
		vga_cursor_x = 0;
		break;
	case '\n':
		/* Newline, treat it as if a carriage return was also there. */
		vga_cursor_x = 0;
		vga_cursor_y++;
		break;
	case '\t':
		vga_cursor_x += 8 - (vga_cursor_x % 8);
		break;
	default:
		/* If it is a non-printing character, ignore it. */
		if(ch < ' ') {
			break;
		}

		VGA_WRITE_WORD((vga_cursor_y * vga_cols) + vga_cursor_x, ch);
		vga_cursor_x++;
		break;
	}

	/* If we have reached the edge of the screen insert a new line. */
	if(vga_cursor_x >= vga_cols) {
		vga_cursor_x = 0;
		vga_cursor_y++;
	}

	/* If we have reached the bottom of the screen, scroll. */
	if(vga_cursor_y >= vga_lines)
		vga_console_scroll();

	/* Move the hardware cursor to the new position. */
	vga_console_move_csr();
}

/** Main console. */
static console_t vga_console = {
	.putch = vga_console_putch,
};

/** Initialize the VGA console.
 * @param tag		Video tag. */
static inline void init_vga_console(kboot_tag_video_t *tag) {
	vga_mapping = (uint16_t *)((ptr_t)tag->vga.mem_virt);
	vga_cursor_x = tag->vga.x;
	vga_cursor_y = tag->vga.y;
	vga_cols = tag->vga.cols;
	vga_lines = tag->vga.lines;

	main_console = &vga_console;
}

/** Initialize the console.
 * @param tags		Tag list. */
void console_init(kboot_tag_t *tags) {
	kboot_tag_video_t *video;

	/* Initialize the serial port. */
	init_serial_console();

	while(tags->type != KBOOT_TAG_NONE) {
		if(tags->type == KBOOT_TAG_VIDEO) {
			video = (kboot_tag_video_t *)tags;

			switch(video->type) {
			case KBOOT_VIDEO_VGA:
				init_vga_console(video);
				break;
			case KBOOT_VIDEO_LFB:
				fb_init(video);
				break;
			}

			break;
		}

		tags = (kboot_tag_t *)ROUND_UP((ptr_t)tags + tags->size, 8);
	}
}
