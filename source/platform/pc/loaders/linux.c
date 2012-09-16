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
 * @brief		PC platform Linux loader.
 */

#include <x86/linux.h>

#include <lib/utility.h>

#include <pc/bios.h>
#include <pc/memory.h>
#include <pc/vbe.h>

#include <loader.h>

/** Get memory information.
 * @param params	Kernel boot paramters page.
 * @return		Whether any method succeeded. */
static bool get_memory_info(linux_params_t *params) {
	bool success = false;
	uint8_t count = 0;
	bios_regs_t regs;

	/* First try function AX=E820h. */
	bios_regs_init(&regs);
	do {
		regs.eax = 0xE820;
		regs.edx = E820_SMAP;
		regs.ecx = 20;
		regs.edi = BIOS_MEM_BASE;
		bios_interrupt(0x15, &regs);

		/* If CF is set, the call was not successful. BIOSes are
		 * allowed to return a non-zero continuation value in EBX and
		 * return an error on next call to indicate that the end of the
		 * list has been reached. */
		if(regs.eflags & X86_FLAGS_CF)
			break;

		memcpy(&params->e820_map[count], (void *)BIOS_MEM_BASE, sizeof(params->e820_map[count]));
		count++;
	} while(regs.ebx != 0 && count < ARRAY_SIZE(params->e820_map));

	if((params->e820_entries = count))
		success = true;

	/* Try function AX=E801h. */
	bios_regs_init(&regs);
	regs.eax = 0xE801;
	bios_interrupt(0x15, &regs);
	if(!(regs.eflags & X86_FLAGS_CF)) {
		if(regs.cx || regs.dx) {
			regs.ax = regs.cx;
			regs.bx = regs.dx;
		}

		/* Maximum value is 15MB. */
		if(regs.ax <= 0x3C00) {
			success = true;
			if(regs.ax == 0x3C00) {
				params->alt_mem_k += (regs.bx << 6) + regs.ax;
			} else {
				params->alt_mem_k = regs.ax;
			}
		}
	}

	/* Finally try AH=88h. */
	bios_regs_init(&regs);
	regs.eax = 0x8800;
	bios_interrupt(0x15, &regs);
	if(!(regs.eflags & X86_FLAGS_CF)) {
		/* Why this is under screen_info is beyond me... */
		params->screen_info.ext_mem_k = regs.ax;
		success = true;
	}

	return success;
}

/** Get APM BIOS information.
 * @param params	Kernel boot paramters page. */
static void get_apm_info(linux_params_t *params) {
	bios_regs_t regs;

	bios_regs_init(&regs);
	regs.eax = 0x5300;
	bios_interrupt(0x15, &regs);
	if(regs.eflags & X86_FLAGS_CF || regs.bx != 0x504D || !(regs.cx & (1<<1)))
		return;

	/* Connect 32-bit interface. */
	regs.eax = 0x5304;
	bios_interrupt(0x15, &regs);
	regs.eax = 0x5303;
	bios_interrupt(0x15, &regs);
	if(regs.eflags & X86_FLAGS_CF)
		return;

	params->apm_bios_info.cseg = regs.ax;
	params->apm_bios_info.offset = regs.ebx;
	params->apm_bios_info.cseg_16 = regs.cx;
	params->apm_bios_info.dseg = regs.dx;
	params->apm_bios_info.cseg_len = regs.si;
	params->apm_bios_info.cseg_16_len = regs.esi >> 16;
	params->apm_bios_info.dseg_len = regs.di;

	regs.eax = 0x5300;
	bios_interrupt(0x15, &regs);
	if(regs.eflags & X86_FLAGS_CF || regs.bx != 0x504D) {
		/* Failed to connect 32-bit interface, disconnect. */
		regs.eax = 0x5304;
		bios_interrupt(0x15, &regs);
		return;
	}

	params->apm_bios_info.version = regs.ax;
	params->apm_bios_info.flags = regs.cx;
}

/** Get Intel SpeedStep BIOS information.
 * @param params	Kernel boot paramters page. */
static void get_ist_info(linux_params_t *params) {
	bios_regs_t regs;

	bios_regs_init(&regs);
	regs.eax = 0xE980;
	regs.edx = 0x47534943;
	bios_interrupt(0x15, &regs);

	params->ist_info.signature = regs.eax;
	params->ist_info.command = regs.ebx;
	params->ist_info.event = regs.ecx;
	params->ist_info.perf_level = regs.edx;
}

/** Set up the video mode for the kernel.
 * @param params	Kernel boot paramters page. */
static void set_video_mode(linux_params_t *params) {
	bios_regs_t regs;

	/* TODO: VESA mode set. */

	params->screen_info.orig_video_isVGA = LINUX_VIDEO_TYPE_VGA;
	params->screen_info.orig_video_mode = 0x3;
	params->screen_info.orig_video_cols = 80;
	params->screen_info.orig_video_lines = 25;
	params->screen_info.orig_x = 0;
	params->screen_info.orig_y = 0;

	/* Restore the console to a decent state. Set display page to the first
	 * and move the cursor to (0, 0). */
	bios_regs_init(&regs);
	regs.eax = 0x0500;
	bios_interrupt(0x10, &regs);
	bios_regs_init(&regs);
	regs.eax = 0x0200;
	bios_interrupt(0x10, &regs);
}

/**
 * PC-specific Linux kernel environment setup.
 *
 * Since we use the 32-bit boot protocol, it is the job of this function to
 * replicate the work done by the real-mode bootstrap code. This means getting
 * all the information from the BIOS required by the kernel.
 *
 * @param params	Kernel boot parameters page.
 */
void linux_platform_load(linux_params_t *params) {
	if(!get_memory_info(params))
		boot_error("Failed to get Linux memory information");

	get_apm_info(params);
	get_ist_info(params);

	/* Don't bother with EDD and MCA, AFAIK they're not used. */

	set_video_mode(params);
}
