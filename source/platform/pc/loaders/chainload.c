/*
 * Copyright (C) 2010-2012 Alex Smith
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
 * @brief		PC chainload loader type.
 */

#include <arch/io.h>

#include <pc/bios.h>
#include <pc/disk.h>

#include <config.h>
#include <loader.h>

extern void chain_loader_enter(uint8_t id, ptr_t part) __noreturn;

/** Details of where to load stuff to. */
#define CHAINLOAD_ADDR		0x7C00
#define CHAINLOAD_SIZE		512
#define PARTITION_TABLE_ADDR	0x7BE
#define PARTITION_TABLE_OFFSET	446
#define PARTITION_TABLE_SIZE	64

/** Load a chainload entry.
 * @note		Assumes the disk has an MSDOS partition table. */
static __noreturn void chain_loader_load(void) {
	disk_t *disk, *parent;
	ptr_t part_addr = 0;
	file_handle_t *file;
	uint8_t id;
	char *path;

	if(current_device->type != DEVICE_TYPE_DISK)
		boot_error("Cannot chainload from non-disk device");

	disk = (disk_t *)current_device;

	path = current_environ->data;
	if(path) {
		/* Loading from a file. */
		file = file_open(path);
		if(!file)
			boot_error("Could not read boot file");

		/* Read in the boot sector. */
		if(!file_read(file, (void *)CHAINLOAD_ADDR, CHAINLOAD_SIZE, 0))
			boot_error("Could not read boot sector");

		file_close(file);
	} else {
		/* Loading the boot sector from the disk. */
		if(!disk_read(disk, (void *)CHAINLOAD_ADDR, CHAINLOAD_SIZE, 0))
			boot_error("Could not read boot sector");
	}

	/* Check if this is a valid boot sector. */
	if(*(uint16_t *)(CHAINLOAD_ADDR + 510) != 0xAA55)
		boot_error("Boot sector is missing signature");

	/* Get the ID of the disk we're booting from. */
	id = bios_disk_id(disk);
	dprintf("loader: chainloading from device %s (id: 0x%x)\n", current_device->name, id);

	/* If booting a partition, we must give partition information to it. */
	if((parent = disk_parent(disk)) != disk) {
		if(!disk_read(parent, (void *)PARTITION_TABLE_ADDR, PARTITION_TABLE_SIZE,
			PARTITION_TABLE_OFFSET))
		{
			boot_error("Could not read partition table");
		}

		part_addr = PARTITION_TABLE_ADDR + (disk->id << 4);
	}

	/* Drop to real mode and jump to the boot sector. */
	chain_loader_enter(id, part_addr);
}

/** Chainload loader type. */
static loader_type_t chain_loader_type = {
	.load = chain_loader_load,
};

/** Chainload another boot sector.
 * @param args		Arguments for the command.
 * @return		Whether successful. */
static bool config_cmd_chainload(value_list_t *args) {
	char *path = NULL;

	if(args->count != 0 && args->count != 1) {
		dprintf("config: chainload: invalid arguments\n");
		return false;
	}

	if(args->count == 1) {
		if(args->values[0].type != VALUE_TYPE_STRING) {
			dprintf("config: chainload: invalid arguments\n");
			return false;
		}

		path = kstrdup(args->values[0].string);
	}

	current_environ->loader = &chain_loader_type;
	current_environ->data = path;
	return true;
}

BUILTIN_COMMAND("chainload", config_cmd_chainload);
