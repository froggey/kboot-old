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
 * @brief		Framebuffer console.
 */

#include <lib/ctype.h>
#include <lib/string.h>
#include <lib/utility.h>

#include <console.h>

#include "test.h"

extern unsigned char console_font[];

/** Dimensions and colours of the console font. */
#define FONT_WIDTH		6
#define FONT_HEIGHT		12
#define FONT_FG			0xffffff
#define FONT_BG			0x000000

/** Get red colour from RGB value. */
#define RED(x, bits)		((x >> (24 - bits)) & ((1 << bits) - 1))

/** Get green colour from RGB value. */
#define GREEN(x, bits)		((x >> (16 - bits)) & ((1 << bits) - 1))

/** Get blue colour from RGB value. */
#define BLUE(x, bits)		((x >> (8  - bits)) & ((1 << bits) - 1))

/** Get the byte offset of a pixel. */
#define OFFSET(x, y)		((y * video_info->lfb.pitch) + (x * (video_info->lfb.bpp / 8)))

/** Framebuffer information. */
static kboot_tag_video_t *video_info;

/** Framebuffer console information. */
static uint16_t fb_console_cols;
static uint16_t fb_console_lines;
static uint16_t fb_console_x = 0;
static uint16_t fb_console_y = 0;

/** Put a pixel on the framebuffer.
 * @param x		X position.
 * @param y		Y position.
 * @param rgb		RGB colour to draw. */
static void put_pixel(uint16_t x, uint16_t y, uint32_t rgb) {
	uint32_t value = (RED(rgb, video_info->lfb.red_size) << video_info->lfb.red_pos)
		| (GREEN(rgb, video_info->lfb.green_size) << video_info->lfb.green_pos)
		| (BLUE(rgb, video_info->lfb.blue_size) << video_info->lfb.blue_pos);
	ptr_t dest = video_info->lfb.fb_virt + OFFSET(x, y);

	switch(video_info->lfb.bpp) {
	case 15:
	case 16:
		*(uint16_t *)dest = (uint16_t)value;
		break;
	case 24:
		((uint8_t *)dest)[0] = value & 0xff;
		((uint8_t *)dest)[1] = (value >> 8) & 0xff;
		((uint8_t *)dest)[2] = (value >> 16) & 0xff;
		break;
	case 32:
		*(uint32_t *)dest = value;
		break;
	}
}

/** Draw a rectangle in a solid colour.
 * @param x		X position of rectangle.
 * @param y		Y position of rectangle.
 * @param width		Width of rectangle.
 * @param height	Height of rectangle.
 * @param rgb		Colour to draw in. */
static void fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t rgb) {
	uint16_t i, j;

	for(i = 0; i < height; i++) {
		for(j = 0; j < width; j++) {
			put_pixel(x + j, y + i, rgb);
		}
	}
}

/** Copy part of the framebuffer to another location.
 * @param dest_y	Y position of destination.
 * @param src_y		Y position of source area.
 * @param height	Height of area to copy. */
static void copy_lines(uint16_t dest_y, uint16_t src_y, uint16_t height) {
	void *mapping = (void *)((ptr_t)video_info->lfb.fb_virt);

	memmove(mapping + OFFSET(0, dest_y), mapping + OFFSET(0, src_y),
		video_info->lfb.pitch * height);
}

/** Draw the glyph at the specified position the console.
 * @param ch		Character to draw.
 * @param x		X position (characters).
 * @param y		Y position (characters).
 * @param fg		Foreground colour.
 * @param bg		Background colour. */
static void draw_glyph(unsigned char ch, uint16_t x, uint16_t y, uint32_t fg, uint32_t bg) {
	uint16_t i, j;

	/* Convert to a pixel position. */
	x *= FONT_WIDTH;
	y *= FONT_HEIGHT;

	/* Draw the glyph. */
	for(i = 0; i < FONT_HEIGHT; i++) {
		for(j = 0; j < FONT_WIDTH; j++) {
			if(console_font[(ch * FONT_HEIGHT) + i] & (1<<(7-j))) {
				put_pixel(x + j, y + i, fg);
			} else {
				put_pixel(x + j, y + i, bg);
			}
		}
	}
}

/** Write a character to the framebuffer console.
 * @param ch		Character to write. */
static void fb_console_putch(char ch) {
	switch(ch) {
	case '\b':
		/* Backspace, move back one character if we can. */
		if(fb_console_x) {
			fb_console_x--;
		} else if(fb_console_y) {
			fb_console_x = fb_console_cols - 1;
			fb_console_y--;
		}
		break;
	case '\r':
		/* Carriage return, move to the start of the line. */
		fb_console_x = 0;
		break;
	case '\n':
		/* Newline, treat it as if a carriage return was there (will
		 * be handled below). */
		fb_console_x = fb_console_cols;
		break;
	case '\t':
		fb_console_x += 8 - (fb_console_x % 8);
		break;
	default:
		/* If it is a non-printing character, ignore it. */
		if(ch < ' ') {
			break;
		}

		draw_glyph(ch, fb_console_x, fb_console_y, FONT_FG, FONT_BG);
		fb_console_x++;
		break;
	}

	/* If we have reached the edge of the screen insert a new line. */
	if(fb_console_x >= fb_console_cols) {
		fb_console_x = 0;
		fb_console_y++;
	}

	/* If we have reached the bottom of the screen, scroll. */
	if(fb_console_y >= fb_console_lines) {
		/* Move everything up and fill the last row with blanks. */
		copy_lines(0, FONT_HEIGHT, (fb_console_lines - 1) * FONT_HEIGHT);
		fill_rect(0, FONT_HEIGHT * (fb_console_lines - 1),
			video_info->lfb.width, FONT_HEIGHT,
			FONT_BG);

		/* Update the cursor position. */
		fb_console_y = fb_console_lines - 1;
	}
}

/** Framebuffer console operations. */
static console_t fb_console = {
	.putch = fb_console_putch,
};

/** Initialize the framebuffer console.
 * @param tag		Video tag. */
void fb_init(kboot_tag_video_t *tag) {
	video_info = tag;

	fb_console_x = fb_console_y = 0;
	fb_console_cols = video_info->lfb.width / FONT_WIDTH;
	fb_console_lines = video_info->lfb.height / FONT_HEIGHT;

	/* Clear to the font background colour. */
	fill_rect(0, 0, video_info->lfb.width, video_info->lfb.height, FONT_BG);

	main_console = &fb_console;
}
