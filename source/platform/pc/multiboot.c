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
 * @brief		Multiboot support.
 */

#include <lib/list.h>
#include <lib/string.h>
#include <lib/utility.h>

#include <pc/multiboot.h>

#include <config.h>
#include <device.h>
#include <fs.h>
#include <memory.h>

/** Multiboot FS file information. */
typedef struct multiboot_file {
	list_t header;			/**< Link to files list. */

	void *addr;			/**< Module data address. */
	size_t size;			/**< Module data size. */
	char *name;			/**< Module name. */
} multiboot_file_t;

/** List of files loaded by Multiboot. */
static LIST_DECLARE(multiboot_files);

/** Read from a file.
 * @param handle	Handle to the file.
 * @param buf		Buffer to read into.
 * @param count		Number of bytes to read.
 * @param offset	Offset into the file.
 * @return		Whether read successfully. */
static bool multiboot_read(file_handle_t *handle, void *buf, size_t count, offset_t offset) {
	multiboot_file_t *file = handle->data;

	if(offset >= (offset_t)file->size) {
		return false;
	} else if((offset + count) > (offset_t)file->size) {
		return false;
	}

	memcpy(buf, file->addr + (size_t)offset, count);
	return true;
}

/** Get the size of a file.
 * @param handle	Handle to the file.
 * @return		Size of the file. */
static offset_t multiboot_size(file_handle_t *handle) {
	multiboot_file_t *file = handle->data;
	return file->size;
}

/** Read directory entries.
 * @param handle	Handle to directory.
 * @param cb		Callback to call on each entry.
 * @param arg		Data to pass to callback.
 * @return		Whether read successfully. */
static bool multiboot_iterate(file_handle_t *handle, dir_iterate_cb_t cb, void *arg) {
	multiboot_file_t *file;
	file_handle_t *child;
	bool ret;

	LIST_FOREACH(&multiboot_files, iter) {
		file = list_entry(iter, multiboot_file_t, header);

		child = file_handle_create(handle->mount, false, file);
		ret = cb(file->name, child, arg);
		file_close(child);
		if(!ret)
			break;
	}

	return true;
}

/** Multiboot filesystem type. */
static fs_type_t multiboot_fs_type = {
	.read = multiboot_read,
	.size = multiboot_size,
	.iterate = multiboot_iterate,
};

/** Perform initialization required when booting from Multiboot. */
void multiboot_init(void) {
	multiboot_module_t *modules;
	multiboot_file_t *file;
	char *tok, *cmdline;
	device_t *device;
	phys_ptr_t addr;
	mount_t *mount;
	uint32_t i;

	if(multiboot_magic != MB_LOADER_MAGIC)
		return;

	/* Parse command line options. */
	cmdline = (char *)multiboot_info.cmdline;
	while((tok = strsep(&cmdline, " "))) {
		if(strncmp(tok, "config-file=", 12) == 0) {
			config_file_override = tok + 12;
		}
	}

	/* If we were given modules, they form the boot filesystem. */
	if(multiboot_info.mods_count) {
		dprintf("loader: using Multiboot modules as boot FS (addr: %p, count: %u)\n",
			multiboot_info.mods_addr, multiboot_info.mods_count);

		modules = (multiboot_module_t *)multiboot_info.mods_addr;
		for(i = 0; i < multiboot_info.mods_count; i++) {
			if(!modules[i].cmdline)
				continue;

			file = kmalloc(sizeof(multiboot_file_t));
			list_init(&file->header);
			file->size = modules[i].mod_end - modules[i].mod_start;

			/* We only want the base name, take off any path strings. */
			file->name = strrchr((char *)modules[i].cmdline, '/');
			file->name = (file->name) ? file->name + 1 : (char *)modules[i].cmdline;
			/* Work around bug in Grub2, it doesn't seem to be
			 * passing the module name in properly. */
			file->name = (char *)"loader.cfg";

			/* We now need to re-allocate the module data as high
			 * as possible so as to make it unlikely that we will
			 * conflict with fixed load addresses for a kernel. */
			phys_memory_alloc(ROUND_UP(file->size, PAGE_SIZE), 0,
				0, 0, PHYS_MEMORY_INTERNAL, PHYS_ALLOC_HIGH,
				&addr);
			file->addr = (void *)P2V(addr);
			memcpy(file->addr, (void *)P2V(modules[i].mod_start), file->size);

			list_append(&multiboot_files, &file->header);
		}

		/* Create a filesystem. */
		mount = kmalloc(sizeof(mount_t));
		mount->type = &multiboot_fs_type;
		mount->root = file_handle_create(mount, true, NULL);
		mount->label = kstrdup("Multiboot");
		mount->uuid = NULL;
		device = kmalloc(sizeof(device_t));
		device_add(device, "multiboot", DEVICE_TYPE_IMAGE);
		device->fs = mount;

		/* This is the boot device. */
		boot_device = device;
	}
}
