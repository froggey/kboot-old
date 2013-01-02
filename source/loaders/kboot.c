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
 * @brief		KBoot kernel loader.
 *
 * There are 3 forms of the 'kboot' configuration command:
 *  - kboot <kernel path> <module list>
 *    Loads the specified kernel and all modules specified in the given list.
 *  - kboot <kernel path> <module dir>
 *    Loads the specified kernel and all modules in the given directory.
 *  - kboot <kernel path>
 *    Loads the specified kernel and no modules.
 *
 * @todo		Add a root_device configuration variable to specify
 *			a different device to pass as boot device to kernel,
 *			which would allow a separate boot partition and root
 *			FS.
 */

#include <lib/string.h>
#include <lib/utility.h>

#include <loaders/kboot.h>

#include <assert.h>
#include <config.h>
#include <fs.h>
#include <loader.h>
#include <memory.h>
#include <net.h>
#include <ui.h>

/** Structure describing a virtual memory mapping. */
typedef struct virt_mapping {
	list_t header;			/**< List header. */

	kboot_vaddr_t start;		/**< Start of the virtual memory range. */
	kboot_vaddr_t size;		/**< Size of the virtual memory range. */
	kboot_paddr_t phys;		/**< Physical address that this range maps to. */
} virt_mapping_t;

#ifdef KBOOT_LOG_BUFFER
/** Whether the log buffer has been allocated. */
static bool log_buffer_allocated = false;
#endif

/** Add an image tag to the image tag list.
 * @param loader	Loader to add to.
 * @param type		Type of the tag.
 * @param size		Size of the tag.
 * @return		Address of created tag. Will be cleared to 0. */
static inline void *add_image_tag(kboot_loader_t *loader, uint32_t type, size_t size) {
	kboot_itag_t *tag;

	tag = kmalloc(sizeof(kboot_itag_t) + size);
	list_init(&tag->header);
	tag->type = type;
	memset(&tag[1], 0, size);
	list_append(&loader->itags, &tag->header);
	return &tag[1];
}

/** Allocate a tag list entry.
 * @param loader	KBoot loader data structure.
 * @param type		Type of the tag.
 * @param size		Size of the tag data.
 * @return		Pointer to allocated tag. Will be cleared to 0. */
void *kboot_allocate_tag(kboot_loader_t *loader, uint32_t type, size_t size) {
	kboot_tag_core_t *core = (kboot_tag_core_t *)((ptr_t)loader->tags_phys);
	kboot_tag_t *ret;

	ret = (kboot_tag_t *)((ptr_t)loader->tags_phys + core->tags_size);
	memset(ret, 0, size);
	ret->type = type;
	ret->size = size;

	core->tags_size += ROUND_UP(size, 8);
	if(core->tags_size > PAGE_SIZE)
		internal_error("Exceeded maximum tag list size");

	return ret;
}

/** Insert a virtual address mapping.
 * @param loader	KBoot loader data structure.
 * @param start		Virtual address of start of mapping.
 * @param size		Size of the mapping.
 * @param phys		Physical address. */
static void add_virt_mapping(kboot_loader_t *loader, kboot_vaddr_t start,
	kboot_vaddr_t size, kboot_vaddr_t phys)
{
	virt_mapping_t *mapping, *other;

	/* All virtual memory tags should be provided together in the tag list,
	 * sorted in address order. To do this, we must maintain mapping info
	 * separately in sorted order, then add it all to the tag list at once. */
	mapping = kmalloc(sizeof(*mapping));
	list_init(&mapping->header);
	mapping->start = start;
	mapping->size = size;
	mapping->phys = phys;

	LIST_FOREACH(&loader->mappings, iter) {
		other = list_entry(iter, virt_mapping_t, header);
		if(mapping->start <= other->start) {
			list_add_before(&other->header, &mapping->header);
			break;
		}
	}

	if(list_empty(&mapping->header))
		list_append(&loader->mappings, &mapping->header);
}

/** Allocate space in the virtual address space.
 * @param loader	KBoot loader data structure.
 * @param phys		Physical address to map to (or ~0 for no mapping).
 * @param size		Size of the range.
 * @return		Virtual address of mapping. */
kboot_vaddr_t kboot_allocate_virtual(kboot_loader_t *loader, kboot_paddr_t phys,
	kboot_vaddr_t size)
{
	kboot_vaddr_t addr;

	if(!allocator_alloc(&loader->alloc, size, 0, &addr))
		boot_error("Unable to allocate %zu bytes of virtual address space", size);

	if(phys != ~(kboot_paddr_t)0)
		mmu_map(loader->mmu, addr, phys, size);

	add_virt_mapping(loader, addr, size, phys);
	return addr;
}

/** Map at a location in the virtual address space.
 * @param loader	KBoot loader data structure.
 * @param addr		Virtual address to map at.
 * @param phys		Physical address to map to (or ~0 for no mapping).
 * @param size		Size of the range. */
void kboot_map_virtual(kboot_loader_t *loader, kboot_vaddr_t addr, kboot_paddr_t phys,
	kboot_vaddr_t size)
{
	if(!allocator_insert(&loader->alloc, addr, size))
		boot_error("Specified mapping conflicts with another");

	if(phys != ~(kboot_paddr_t)0)
		mmu_map(loader->mmu, addr, phys, size);

	add_virt_mapping(loader, addr, size, phys);
}

/** Load a single module.
 * @param loader	KBoot loader data structure.
 * @param handle	Handle to module to load.
 * @param name		Name of the module. */
static void load_module(kboot_loader_t *loader, file_handle_t *handle, const char *name) {
	kboot_tag_module_t *tag;
	phys_ptr_t addr;
	offset_t size;

	if(handle->directory)
		return;

	kprintf("Loading %s...\n", name);

	/* Allocate a chunk of memory to load to. */
	size = file_size(handle);
	phys_memory_alloc(ROUND_UP(size, PAGE_SIZE), 0, 0, 0, PHYS_MEMORY_MODULES,
		0, &addr);
	if(!file_read(handle, (void *)((ptr_t)addr), size, 0))
		boot_error("Could not read module `%s'", name);

	/* Add the module to the tag list. */
	tag = kboot_allocate_tag(loader, KBOOT_TAG_MODULE, sizeof(*tag));
	tag->addr = addr;
	tag->size = size;

	dprintf("kboot: loaded module %s to 0x%" PRIxPHYS " (size: %" PRIu64 ")\n",
		name, addr, size);
}

/** Load a list of modules.
 * @param loader	KBoot loader data structure.
 * @param list		List to load. */
static void load_module_list(kboot_loader_t *loader, value_list_t *list) {
	file_handle_t *handle;
	size_t i;

	for(i = 0; i < list->count; i++) {
		handle = file_open(list->values[i].string, NULL);
		if(!handle)
			boot_error("Could not open module %s", list->values[i].string);

		load_module(loader, handle, strrchr(list->values[i].string, '/') + 1);
		file_close(handle);
	}
}

/** Callback to load a module from a directory.
 * @param name		Name of the entry.
 * @param handle	Handle to entry.
 * @param _loader	KBoot loader data structure.
 * @return		Whether to continue iteration. */
static bool load_modules_cb(const char *name, file_handle_t *handle, void *_loader) {
	load_module(_loader, handle, name);
	return true;
}

/** Load a directory of modules.
 * @param loader	KBoot loader data structure.
 * @param path		Path to directory. */
static void load_module_dir(kboot_loader_t *loader, const char *path) {
	file_handle_t *handle;

	handle = file_open(path, NULL);
	if(!handle) {
		boot_error("Could not find module directory `%s'", path);
	} else if(!handle->directory) {
		boot_error("Module directory `%s' not directory", path);
	} else if(!handle->mount->type->iterate) {
		boot_error("Cannot use module directory on non-listable FS");
	}

	if(!dir_iterate(handle, load_modules_cb, loader))
		boot_error("Failed to iterate module directory");

	file_close(handle);
}

/** Set a single option.
 * @param loader	KBoot loader data structure.
 * @param name		Name of the option.
 * @param type		Type of the option. */
static void set_option(kboot_loader_t *loader, const char *name, uint32_t type) {
	size_t name_size, value_size;
	kboot_tag_option_t *tag;
	value_t *env;
	void *value;

	name_size = strlen(name) + 1;

	env = environ_lookup(current_environ, name);
	switch(type) {
	case KBOOT_OPTION_BOOLEAN:
		value = &env->boolean;
		value_size = 1;
		break;
	case KBOOT_OPTION_STRING:
		value = env->string;
		value_size = strlen(env->string) + 1;
		break;
	case KBOOT_OPTION_INTEGER:
		value = &env->integer;
		value_size = sizeof(uint64_t);
		break;
	default:
		internal_error("Shouldn't get here");
	}

	tag = kboot_allocate_tag(loader, KBOOT_TAG_OPTION, ROUND_UP(sizeof(*tag), 8)
		+ ROUND_UP(name_size, 8) + value_size);
	tag->type = type;
	tag->name_size = name_size;
	tag->value_size = value_size;

	memcpy((char *)tag + ROUND_UP(sizeof(*tag), 8), name, name_size);
	memcpy((char *)tag + ROUND_UP(sizeof(*tag), 8) + ROUND_UP(name_size, 8),
		value, value_size);
}

/** Add boot device information to the tag list.
 * @param loader	KBoot loader data structure. */
static void add_bootdev_tag(kboot_loader_t *loader) {
	kboot_tag_bootdev_t *tag;
	net_device_t *netdev;
	disk_t *disk;

	tag = kboot_allocate_tag(loader, KBOOT_TAG_BOOTDEV, sizeof(*tag));

	switch(current_device->type) {
	case DEVICE_TYPE_DISK:
		disk = (disk_t *)current_device;

		tag->type = KBOOT_BOOTDEV_DISK;
		tag->disk.flags = 0;

		if(current_device->fs->uuid) {
			strncpy((char *)tag->disk.uuid, current_device->fs->uuid,
				sizeof(tag->disk.uuid));
		} else {
			tag->disk.uuid[0] = 0;
		}

		/* Copy disk device identification information. The ID field in
		 * the disk structure is either a platform-specific ID or a
		 * partition ID. */
		if(disk->parent) {
			if(disk->parent->parent) {
				tag->disk.sub_partition = disk->id;
				tag->disk.partition = disk->parent->id;
				tag->disk.device = disk->parent->parent->id;
			} else {
				tag->disk.sub_partition = 0;
				tag->disk.partition = disk->id;
				tag->disk.device = disk->parent->id;
			}
		} else {
			tag->disk.sub_partition = 0;
			tag->disk.partition = 0;
			tag->disk.device = disk->id;
		}

		break;
	case DEVICE_TYPE_NET:
		netdev = (net_device_t *)current_device;

		tag->type = KBOOT_BOOTDEV_NET;
		tag->net.flags = (netdev->flags & NET_DEVICE_IPV6) ? KBOOT_NET_IPV6 : 0;
		tag->net.server_port = netdev->server_port;
		tag->net.hw_type = netdev->hw_type;
		tag->net.hw_addr_len = netdev->hw_addr_len;
		memcpy(&tag->net.server_ip, &netdev->server_ip, sizeof(tag->net.server_ip));
		memcpy(&tag->net.gateway_ip, &netdev->gateway_ip, sizeof(tag->net.gateway_ip));
		memcpy(&tag->net.client_ip, &netdev->client_ip, sizeof(tag->net.client_ip));
		memcpy(&tag->net.client_mac, &netdev->client_mac, sizeof(tag->net.client_mac));
		break;
	case DEVICE_TYPE_IMAGE:
		tag->type = KBOOT_BOOTDEV_NONE;
		break;
	}
}

#ifdef KBOOT_LOG_BUFFER

/** Add kernel log information to the tag list.
 * @param loader	KBoot loader data structure. */
static void add_log_tag(kboot_loader_t *loader) {
	kboot_log_t *log = (kboot_log_t *)KBOOT_LOG_BUFFER;
	kboot_tag_log_t *tag;

	/* If the log buffer could not be allocated, do not pass log
	 * information to the kernel. */
	if(!log_buffer_allocated)
		return;

	tag = kboot_allocate_tag(loader, KBOOT_TAG_LOG, sizeof(*tag));
	tag->log_phys = KBOOT_LOG_BUFFER;
	tag->log_size = KBOOT_LOG_SIZE;
	tag->log_virt = kboot_allocate_virtual(loader, tag->log_phys, tag->log_size);
	tag->prev_phys = 0;
	tag->prev_size = 0;

	if(log->magic == loader->log_magic) {
		/* There is an existing log, copy it for the kernel. */
		if(phys_memory_alloc(KBOOT_LOG_SIZE, 0, 0, 0, PHYS_MEMORY_RECLAIMABLE,
			0, &tag->prev_phys))
		{
			tag->prev_size = KBOOT_LOG_SIZE;
			memcpy((void *)((ptr_t)tag->prev_phys), (void *)KBOOT_LOG_BUFFER,
				KBOOT_LOG_SIZE);
		}
	}

	/* Initialize the log buffer. */
	log->magic = loader->log_magic;
	log->start = 0;
	log->length = 0;
	log->info[0] = log->info[1] = log->info[2] = 0;
}

#endif

/** Add virtual memory tags to the tag list.
 * @param loader	KBoot loader data structure. */
static void add_vmem_tags(kboot_loader_t *loader) {
	virt_mapping_t *mapping;
	kboot_tag_vmem_t *tag;

	dprintf("kboot: final virtual memory map:\n");

	LIST_FOREACH(&loader->mappings, iter) {
		mapping = list_entry(iter, virt_mapping_t, header);

		tag = kboot_allocate_tag(loader, KBOOT_TAG_VMEM, sizeof(*tag));
		tag->start = mapping->start;
		tag->size = mapping->size;
		tag->phys = mapping->phys;

		dprintf(" 0x%" PRIx64 "-0x%" PRIx64 " -> 0x%" PRIx64 "\n", tag->start,
			tag->start + tag->size, tag->phys);
	}
}

/** Add physical memory tags to the tag list.
 * @param loader	KBoot loader data structure. */
static void add_memory_tags(kboot_loader_t *loader) {
	kboot_tag_memory_t *tag;
	memory_range_t *range;

	/* Reclaim all memory used internally. */
	memory_finalize();

	/* Add tags for each range. */
	LIST_FOREACH(&memory_ranges, iter) {
		range = list_entry(iter, memory_range_t, header);

		tag = kboot_allocate_tag(loader, KBOOT_TAG_MEMORY, sizeof(*tag));
		tag->start = range->start;
		tag->size = range->size;
		tag->type = range->type;
	}
}

/** Load the operating system. */
static __noreturn void kboot_loader_load(void) {
	kboot_loader_t *loader = current_environ->data;
	ptr_t loader_start, loader_size;
	kboot_itag_load_t *load;
	kboot_tag_core_t *core;

	/* These errors are detected in config_cmd_kboot(), but shouldn't be
	 * reported until the user actually tries to boot the entry. */
	if(!loader->kernel) {
		boot_error("Could not find kernel image");
	} else if(!loader->image) {
		boot_error("Kernel is not a valid KBoot kernel");
	}

	/* Create the tag list. It will be mapped into virtual memory later,
	 * as we cannot yet perform virtual allocations. For now, assume that
	 * the tag list never exceeds a page, which is probably reasonable. */
	phys_memory_alloc(PAGE_SIZE, 0, 0, 0, PHYS_MEMORY_RECLAIMABLE, 0, &loader->tags_phys);
	core = (kboot_tag_core_t *)((ptr_t)loader->tags_phys);
	memset(core, 0, sizeof(kboot_tag_core_t));
	core->header.type = KBOOT_TAG_CORE;
	core->header.size = sizeof(kboot_tag_core_t);
	core->tags_phys = loader->tags_phys;
	core->tags_size = ROUND_UP(sizeof(kboot_tag_core_t), 8);

	/* Check the image and determine the target type. */
	kboot_arch_check(loader);

	/* Validate the load parameters. If there is no load tag specified in
	 * the image, add one and initialize everything to 0. */
	load = kboot_itag_find(loader, KBOOT_ITAG_LOAD);
	if(load) {
		if(!(load->flags & KBOOT_LOAD_FIXED)) {
			if((load->alignment && (load->alignment < PAGE_SIZE
					|| !IS_POW2(load->alignment)))
				|| (load->min_alignment && (load->min_alignment < PAGE_SIZE
					|| load->min_alignment > load->alignment
					|| !IS_POW2(load->min_alignment)))) {
				boot_error("Kernel specifies invalid alignment parameters");
			}

			if(!load->min_alignment)
				load->min_alignment = load->alignment;
		}

		if(loader->target == TARGET_TYPE_32BIT && !load->virt_map_base && !load->virt_map_size)
			load->virt_map_size = 0x100000000ULL;

		if(load->virt_map_base % PAGE_SIZE || load->virt_map_size % PAGE_SIZE
			|| (load->virt_map_base && !load->virt_map_size)
			|| (load->virt_map_base + load->virt_map_size - 1)
				< load->virt_map_base
			|| (loader->target == TARGET_TYPE_32BIT
				&& (load->virt_map_base >= 0x100000000ULL
					|| load->virt_map_base + load->virt_map_size
						> 0x100000000ULL))) {
			boot_error("Kernel specifies invalid virtual map range");
		}
	} else {
		load = add_image_tag(loader, KBOOT_ITAG_LOAD, sizeof(kboot_itag_load_t));
	}

	/* Have the architecture do its own validation and fill in defaults. */
	kboot_arch_load_params(loader, load);

	/* Create the virtual address space and address allocator. Try to
	 * reserve a page to ensure that we never allocate virtual address 0. */
	loader->mmu = mmu_context_create(loader->target);
	allocator_init(&loader->alloc, load->virt_map_base, load->virt_map_size);
	allocator_reserve(&loader->alloc, 0, PAGE_SIZE);

	/* Load the kernel image. */
	kprintf("Loading kernel...\n");
	kboot_elf_load_kernel(loader, load);

	/* Now we need to perform all mappings specified by the image tags. */
	KBOOT_ITAG_ITERATE(loader, KBOOT_ITAG_MAPPING, kboot_itag_mapping_t, mapping) {
		if((mapping->virt != ~(kboot_vaddr_t)0 && mapping->virt % PAGE_SIZE)
			|| mapping->phys % PAGE_SIZE
			|| mapping->size % PAGE_SIZE)
		{
			boot_error("Kernel specifies invalid virtual mapping");
		}

		if(mapping->virt == ~(kboot_vaddr_t)0) {
			/* Allocate an address to map at. */
			kboot_allocate_virtual(loader, mapping->phys, mapping->size);
		} else {
			kboot_map_virtual(loader, mapping->virt, mapping->phys, mapping->size);
		}
	}

	/* Do architecture-specific setup, e.g. for page tables. */
	kboot_arch_setup(loader);

	/* Now we can allocate a virtual mapping for the tag list. */
	loader->tags_virt = kboot_allocate_virtual(loader, core->tags_phys, PAGE_SIZE);

	/* Pass all of the option values. */
	KBOOT_ITAG_ITERATE(loader, KBOOT_ITAG_OPTION, kboot_itag_option_t, option) {
		set_option(loader, (char *)option + sizeof(*option), option->type);
	}

	/* Load modules. */
	if(loader->modules.type == VALUE_TYPE_LIST) {
		load_module_list(loader, loader->modules.list);
	} else if(loader->modules.type == VALUE_TYPE_STRING) {
		load_module_dir(loader, loader->modules.string);
	}

	/* Load additional sections if requested. */
	if(loader->image->flags & KBOOT_IMAGE_SECTIONS)
		kboot_elf_load_sections(loader);

	/* Add the boot device information. */
	add_bootdev_tag(loader);

	#ifdef KBOOT_LOG_BUFFER
	/* Set up the kernel log. */
	add_log_tag(loader);
	#endif

	/* Perform pre-boot tasks. */
	loader_preboot();

	/* Do platform-specific setup, including setting the video mode. */
	kboot_platform_setup(loader);

	/* Create a stack for the kernel. */
	phys_memory_alloc(PAGE_SIZE, 0, 0, 0, PHYS_MEMORY_STACK, 0, &core->stack_phys);
	core->stack_base = loader->stack_virt = kboot_allocate_virtual(loader,
		core->stack_phys, PAGE_SIZE);
	core->stack_size = loader->stack_size = PAGE_SIZE;

	/* Now we have the interesting task of setting things up so that we
	 * can enter the kernel. It is not always possible to identity map the
	 * boot loader: it is possible that something has been mapped into the
	 * virtual address space at the identity mapped location. So, the
	 * procedure we use to enter the kernel is as follows:
	 *  - Allocate a page of the virtual address space, ensuring it does
	 *    not conflict with the physical addresses of the loader.
	 *  - Construct a temporary address space that identity maps the loader
	 *    and the allocated page.
	 *  - Architecture entry code copies a piece of trampoline code to the
	 *    page, then enables the MMU and switches to the target operating
	 *    mode using the temporary address space.
	 *  - Jump to the trampoline code which switches to the real address
	 *    space and then jumps to the kernel.
	 * FIXME: Should mark all allocated page tables as internal so the
	 * kernel won't see them as in use at all. */
	loader_start = ROUND_DOWN((ptr_t)__start, PAGE_SIZE);
	loader_size = ROUND_UP((ptr_t)__end - (ptr_t)__start, PAGE_SIZE);
	allocator_reserve(&loader->alloc, loader_start, loader_size);
	phys_memory_alloc(PAGE_SIZE, 0, 0, 0, PHYS_MEMORY_INTERNAL, 0, &loader->trampoline_phys);
	loader->trampoline_virt = kboot_allocate_virtual(loader, loader->trampoline_phys,
		PAGE_SIZE);
	loader->transition = mmu_context_create(loader->target);
	mmu_map(loader->transition, loader_start, loader_start, loader_size);
	mmu_map(loader->transition, loader->trampoline_phys, loader->trampoline_phys,
		PAGE_SIZE);
	mmu_map(loader->transition, loader->trampoline_virt, loader->trampoline_phys,
		PAGE_SIZE);

	/* All virtual memory allocations should now have been done, we can
	 * insert the final set of sorted virtual memory tags. */
	add_vmem_tags(loader);

	/* Add physical memory information. */
	add_memory_tags(loader);

	/* End the tag list. */
	kboot_allocate_tag(loader, KBOOT_TAG_NONE, sizeof(kboot_tag_t));

	/* Start the kernel. */
	dprintf("kboot: entering kernel at 0x%" PRIx64 " (stack: 0x%" PRIx64 ", "
		"trampoline_phys: 0x%" PRIxPHYS ", trampoline_virt: 0x%" PRIx64 ")\n",
		loader->entry, loader->stack_virt, loader->trampoline_phys,
		loader->trampoline_virt);
	kboot_arch_enter(loader);
}

#if CONFIG_KBOOT_UI

/** Return a window for configuring the OS.
 * @return		Pointer to configuration window. */
static ui_window_t *kboot_loader_configure(void) {
	kboot_loader_t *loader = current_environ->data;
	return (!ui_list_empty(loader->config)) ? loader->config : NULL;
}

#endif

/** KBoot loader type. */
static loader_type_t kboot_loader_type = {
	.load = kboot_loader_load,
	#if CONFIG_KBOOT_UI
	.configure = kboot_loader_configure,
	#endif
};

/** Tag iterator to add options to the environment.
 * @param note		Note header.
 * @param desc		Note data.
 * @param loader	KBoot loader data structure.
 * @return		Whether to continue iteration. */
static bool add_image_tags(elf_note_t *note, void *desc, kboot_loader_t *loader) {
	size_t size;
	void *tag;

	/* Validate the tag type and check that we don't have more than one of
	 * certain tag types. */
	switch(note->n_type) {
	case KBOOT_ITAG_IMAGE:
		if(kboot_itag_find(loader, KBOOT_ITAG_IMAGE)) {
			dprintf("kboot: warning: ignoring duplicate KBOOT_ITAG_IMAGE tag\n");
			return true;
		}

		size = sizeof(kboot_itag_image_t);
		break;
	case KBOOT_ITAG_LOAD:
		if(kboot_itag_find(loader, KBOOT_ITAG_LOAD)) {
			dprintf("kboot: warning: ignoring duplicate KBOOT_ITAG_LOAD tag\n");
			return true;
		}

		size = sizeof(kboot_itag_load_t);
		break;
	case KBOOT_ITAG_VIDEO:
		if(kboot_itag_find(loader, KBOOT_ITAG_VIDEO)) {
			dprintf("kboot: warning: ignoring duplicate KBOOT_ITAG_VIDEO tag\n");
			return true;
		}

		size = sizeof(kboot_itag_video_t);
		break;
	case KBOOT_ITAG_OPTION:
		size = sizeof(kboot_itag_option_t);
		break;
	case KBOOT_ITAG_MAPPING:
		size = sizeof(kboot_itag_mapping_t);
		break;
	default:
		dprintf("kboot: warning: unrecognized image tag type %" PRIu32 "\n", note->n_type);
		return true;
	}

	/* Add to the tag list. */
	tag = add_image_tag(loader, note->n_type, MAX(size, note->n_descsz));
	memcpy(tag, desc, note->n_descsz);

	return true;
}

#ifdef KBOOT_LOG_BUFFER

/** Calculate the magic number for the log buffer.
 * @return		Magic number for the log buffer. */
static uint32_t calculate_log_magic(void) {
	uint32_t magic;

	/* Determine the magic number of the log buffer. This is a rather crude
	 * method to deal with having multiple KBoot OSes on one machine: we
	 * base the magic number on the device identification information. This
	 * means that an existing log will only be shown for the OS it came
	 * from. */
	magic = KBOOT_MAGIC + current_device->type;
	#if CONFIG_KBOOT_HAVE_DISK
	if(current_device->type == DEVICE_TYPE_DISK) {
		disk_t *disk = (disk_t *)current_device;
		while(disk) {
			magic += disk->id;
			disk = disk->parent;
		}
	}
	#endif

	return magic;
}

#if CONFIG_KBOOT_UI

/** Add a viewer for the kernel log.
 * @param loader	KBoot loader data structure. */
static void init_log_viewer(kboot_loader_t *loader) {
	kboot_log_t *log = (kboot_log_t *)KBOOT_LOG_BUFFER;
	ui_window_t *window;

	if(log->magic != loader->log_magic)
		return;

	/* This matches our kernel log, add a viewer. */
	window = ui_textview_create("Kernel Log", (char *)log->buffer,
		KBOOT_LOG_SIZE - sizeof(kboot_log_t), log->start,
		log->length);
	ui_list_insert(loader->config, ui_link_create(window), false);
}

#endif

/** Initialize the kernel log.
 * @param loader	KBoot loader data structure. */
static void init_kernel_log(kboot_loader_t *loader) {
	phys_ptr_t addr;

	/* Determine the magic number of the log buffer. */
	loader->log_magic = calculate_log_magic();

	/* Attempt to allocate the kernel log. */
	if(!log_buffer_allocated) {
		if(!phys_memory_alloc(KBOOT_LOG_SIZE, 0, KBOOT_LOG_BUFFER,
			KBOOT_LOG_BUFFER + KBOOT_LOG_SIZE, PHYS_MEMORY_ALLOCATED,
			PHYS_ALLOC_CANFAIL, &addr))
		{
			return;
		}

		log_buffer_allocated = true;
	}

	#if CONFIG_KBOOT_UI
	/* Add a UI viewer to view the previous log. */
	init_log_viewer(loader);
	#endif
}

#endif

/** Load a KBoot kernel and modules.
 * @param args		Command arguments.
 * @return		Whether completed successfully. */
static bool config_cmd_kboot(value_list_t *args) {
	const char *opt_name, *opt_desc;
	kboot_loader_t *loader;
	value_t *entry, value;
	void *opt_default;

	if((args->count != 1 && args->count != 2)
		|| args->values[0].type != VALUE_TYPE_STRING
		|| (args->count == 2 && args->values[1].type != VALUE_TYPE_LIST
			&& args->values[1].type != VALUE_TYPE_STRING)) {
		dprintf("kboot: invalid arguments\n");
		return false;
	}

	loader = kmalloc(sizeof(*loader));
	list_init(&loader->itags);
	list_init(&loader->mappings);

	current_environ->loader = &kboot_loader_type;
	current_environ->data = loader;

	/* Copy the modules value. If there isn't one, just set an empty list. */
	if(args->count == 2) {
		value_copy(&args->values[1], &loader->modules);
	} else {
		value_init(&loader->modules, VALUE_TYPE_LIST);
	}

	#if CONFIG_KBOOT_UI
	loader->config = ui_list_create("Kernel Options", true);
	#endif

	/* Open the kernel image. If it cannot be opened, return immediately.
	 * The error will be reported when the user tries to boot. */
	loader->kernel = file_open(args->values[0].string, NULL);
	if(!loader->kernel)
		return true;

	/* Read in all image tags from the image. */
	kboot_elf_note_iterate(loader, add_image_tags);

	/* Store a pointer to the image tag as it is used frequently. If there
	 * is no image tag, return immediately as this is not a valid KBoot
	 * image. The error will be reported when the user tries to boot. */
	loader->image = kboot_itag_find(loader, KBOOT_ITAG_IMAGE);
	if(!loader->image)
		return true;

	dprintf("kboot: KBoot version %" PRIu32 " image, flags 0x%" PRIx32 "\n",
		loader->image->version, loader->image->flags);

	/* Parse the option tags to add options to the environment and build
	 * the configuration menu. */
	KBOOT_ITAG_ITERATE(loader, KBOOT_ITAG_OPTION, kboot_itag_option_t, option) {
		opt_name = (char *)option + sizeof(*option);
		opt_desc = (char *)option + sizeof(*option) + option->name_len;
		opt_default = (char *)option + sizeof(*option) + option->name_len
			+ option->desc_len;

		switch(option->type) {
		case KBOOT_OPTION_BOOLEAN:
			value.type = VALUE_TYPE_BOOLEAN;
			value.boolean = *(bool *)opt_default;
			break;
		case KBOOT_OPTION_STRING:
			value.type = VALUE_TYPE_STRING;
			value.string = opt_default;
			break;
		case KBOOT_OPTION_INTEGER:
			value.type = VALUE_TYPE_INTEGER;
			value.integer = *(uint64_t *)opt_default;
			break;
		}

		/* Don't overwrite an existing value. */
		entry = environ_lookup(current_environ, opt_name);
		if(!entry || entry->type != value.type)
			entry = environ_insert(current_environ, opt_name, &value);

		#if CONFIG_KBOOT_UI
		ui_list_insert(loader->config, ui_entry_create(opt_desc, entry), false);
		#endif
	}

	#if CONFIG_KBOOT_HAVE_VIDEO
	/* Call the platform to deal with the video tag and add a video mode
	 * chooser to the UI. */
	kboot_platform_video_init(loader);
	#endif

	#ifdef KBOOT_LOG_BUFFER
	if(loader->image->flags & KBOOT_IMAGE_LOG)
		init_kernel_log(loader);
	#endif

	return true;
}

BUILTIN_COMMAND("kboot", config_cmd_kboot);
