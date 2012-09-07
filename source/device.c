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
 * @brief		Device management.
 */

#include <lib/string.h>

#include <config.h>
#include <device.h>
#include <fs.h>
#include <memory.h>

/** Boot device. */
device_t *boot_device = NULL;

/** List of all devices. */
LIST_DECLARE(device_list);

/**
 * Look up a device according to a string.
 *
 * Looks up a device according to the given string. If the string is in the
 * form "(<name>)", then the device will be looked up by its name. Otherwise,
 * the string will be taken as a UUID, and the device containing a filesystem
 * with that UUID will be returned.
 *
 * @param str		String for lookup.
 *
 * @return		Pointer to device structure if found, NULL if not.
 */
device_t *device_lookup(const char *str) {
	char *name = NULL;
	device_t *device;
	size_t len;

	if(str[0] == '(') {
		len = strlen(str);
		if(len < 3 || str[len - 1] != ')')
			return NULL;
		len -= 2;
		name = kmalloc(len);
		memcpy(name, str + 1, len);
		name[len] = 0;
	}

	LIST_FOREACH(&device_list, iter) {
		device = list_entry(iter, device_t, header);

		if(name) {
			if(strcmp(device->name, name) == 0)
				return device;
		} else if(device->fs && device->fs->uuid) {
			if(strcmp(device->fs->uuid, str) == 0)
				return device;
		}
	}

	return NULL;
}

/**
 * Register a device.
 *
 * Registers a new device. Does not set the FS pointer, so this should be set
 * manually after the function returns. If this is the boot device, the caller
 * should set it as the current device itself.
 *
 * @param device	Device structure for the device.
 * @param name		Name of the device (string will be duplicated).
 * @param type		Type of the device.
 */
void device_add(device_t *device, const char *name, device_type_t type) {
	list_init(&device->header);
	device->name = kstrdup(name);
	device->type = type;
	device->fs = NULL;

	list_append(&device_list, &device->header);
}

/** Set the current device.
 * @param args		Argument list.
 * @return		Whether successful. */
static bool config_cmd_device(value_list_t *args) {
	if(args->count != 1 || args->values[0].type != VALUE_TYPE_STRING) {
		dprintf("device: invalid arguments\n");
		return false;
	}

	/* Look up the device. If the device is not found, we leave the pointer
	 * set as NULL. It will be handled later when the user tries to boot. */
	current_environ->device = device_lookup(args->values[0].string);

	return true;
}
BUILTIN_COMMAND("device", config_cmd_device);
