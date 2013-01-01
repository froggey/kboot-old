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
 * @brief		KBoot test kernel.
 */

#include <lib/utility.h>

#include "test.h"

KBOOT_IMAGE(KBOOT_IMAGE_SECTIONS | KBOOT_IMAGE_LOG);
KBOOT_VIDEO(KBOOT_VIDEO_LFB | KBOOT_VIDEO_VGA, 0, 0, 0);
KBOOT_BOOLEAN_OPTION("bool_option", "Boolean Option", true);
KBOOT_STRING_OPTION("string_option", "String Option", "Default Value");
KBOOT_LOAD(0, 0, 0, VIRT_MAP_BASE, VIRT_MAP_SIZE);
KBOOT_MAPPING(PHYS_MAP_BASE, 0, PHYS_MAP_SIZE);

/** Dump a core tag. */
static void dump_core_tag(kboot_tag_core_t *tag) {
	kprintf("KBOOT_TAG_CORE:\n");
	kprintf("  tags_phys   = 0x%" PRIx64 "\n", tag->tags_phys);
	kprintf("  tags_size   = %" PRIu32 "\n", tag->tags_size);
	kprintf("  kernel_phys = 0x%" PRIx64 "\n", tag->kernel_phys);
	kprintf("  stack_base  = 0x%" PRIx64 "\n", tag->stack_base);
	kprintf("  stack_phys  = 0x%" PRIx64 "\n", tag->stack_phys);
	kprintf("  stack_size  = %" PRIu32 "\n", tag->stack_size);
}

/** Dump an option tag. */
static void dump_option_tag(kboot_tag_option_t *tag) {
	const char *name;
	void *value;

	kprintf("KBOOT_TAG_OPTION:\n");
	kprintf("  type       = %" PRIu8 "\n", tag->type);
	kprintf("  name_size  = %" PRIu32 "\n", tag->name_size);
	kprintf("  value_size = %" PRIu32 "\n", tag->value_size);

	name = (const char *)ROUND_UP((ptr_t)tag + sizeof(kboot_tag_option_t), 8);
	kprintf("  name       = `%s'\n", name);

	value = (void *)ROUND_UP((ptr_t)name + tag->name_size, 8);
	switch(tag->type) {
	case KBOOT_OPTION_BOOLEAN:
		kprintf("  value      = boolean: %d\n", *(bool *)value);
		break;
	case KBOOT_OPTION_STRING:
		kprintf("  value      = string: `%s'\n", (const char *)value);
		break;
	case KBOOT_OPTION_INTEGER:
		kprintf("  value      = integer: %" PRIu64 "\n", *(uint64_t *)value);
		break;
	default:
		kprintf("  <unknown type>\n");
		break;
	}
}

/** Get a memory range tag type. */
static const char *memory_tag_type(uint32_t type) {
	switch(type) {
	case KBOOT_MEMORY_FREE:
		return "Free";
	case KBOOT_MEMORY_ALLOCATED:
		return "Allocated";
	case KBOOT_MEMORY_RECLAIMABLE:
		return "Reclaimable";
	case KBOOT_MEMORY_PAGETABLES:
		return "Pagetables";
	case KBOOT_MEMORY_STACK:
		return "Stack";
	case KBOOT_MEMORY_MODULES:
		return "Modules";
	default:
		return "???";
	}
}

/** Dump a memory tag. */
static void dump_memory_tag(kboot_tag_memory_t *tag) {
	kprintf("KBOOT_TAG_MEMORY:\n");
	kprintf("  start = 0x%" PRIx64 "\n", tag->start);
	kprintf("  size  = 0x%" PRIx64 "\n", tag->size);
	kprintf("  end   = 0x%" PRIx64 "\n", tag->start + tag->size);
	kprintf("  type  = %u (%s)\n", tag->type, memory_tag_type(tag->type));
}

/** Dump a virtual memory tag. */
static void dump_vmem_tag(kboot_tag_vmem_t *tag) {
	kprintf("KBOOT_TAG_VMEM:\n");
	kprintf("  start = 0x%" PRIx64 "\n", tag->start);
	kprintf("  size  = 0x%" PRIx64 "\n", tag->size);
	kprintf("  end   = 0x%" PRIx64 "\n", tag->start + tag->size);
	kprintf("  phys  = 0x%" PRIx64 "\n", tag->phys);
}

/** Dump a pagetables tag. */
static void dump_pagetables_tag(kboot_tag_pagetables_t *tag) {
	kprintf("KBOOT_TAG_PAGETABLES:\n");
#ifdef __x86_64__
	kprintf("  pml4    = 0x%" PRIx64 "\n", tag->pml4);
	kprintf("  mapping = 0x%" PRIx64 "\n", tag->mapping);
#elif defined(__i386__)
	kprintf("  page_dir = 0x%" PRIx64 "\n", tag->page_dir);
	kprintf("  mapping  = 0x%" PRIx64 "\n", tag->mapping);
#endif
}

/** Dump a module tag. */
static void dump_module_tag(kboot_tag_module_t *tag) {
	kprintf("KBOOT_TAG_MODULE:\n");
	kprintf("  addr = 0x%" PRIx64 "\n", tag->addr);
	kprintf("  size = %" PRIu32 "\n", tag->size);
}

/** Dump a video tag. */
static void dump_video_tag(kboot_tag_video_t *tag) {
	kprintf("KBOOT_TAG_VIDEO:\n");

	switch(tag->type) {
	case KBOOT_VIDEO_VGA:
		kprintf("  type     = %u (KBOOT_VIDEO_VGA)\n", tag->type);
		kprintf("  cols     = %u\n", tag->vga.cols);
		kprintf("  lines    = %u\n", tag->vga.lines);
		kprintf("  x        = %u\n", tag->vga.x);
		kprintf("  y        = %u\n", tag->vga.y);
		kprintf("  mem_phys = 0x%" PRIx64 "\n", tag->vga.mem_phys);
		kprintf("  mem_virt = 0x%" PRIx64 "\n", tag->vga.mem_virt);
		kprintf("  mem_size = 0x%" PRIx32 "\n", tag->vga.mem_size);
		break;
	case KBOOT_VIDEO_LFB:
		kprintf("  type       = %u (KBOOT_VIDEO_LFB)\n", tag->type);
		kprintf("  flags      = 0x%" PRIx32 "\n", tag->lfb.flags);
		if(tag->lfb.flags & KBOOT_LFB_RGB)
			kprintf("    KBOOT_LFB_RGB\n");
		if(tag->lfb.flags & KBOOT_LFB_INDEXED)
			kprintf("    KBOOT_LFB_INDEXED\n");
		kprintf("  width      = %" PRIu32 "\n", tag->lfb.width);
		kprintf("  height     = %" PRIu32 "\n", tag->lfb.height);
		kprintf("  bpp        = %" PRIu8 "\n", tag->lfb.bpp);
		kprintf("  pitch      = %" PRIu32 "\n", tag->lfb.pitch);
		kprintf("  fb_phys    = 0x%" PRIx64 "\n", tag->lfb.fb_phys);
		kprintf("  fb_virt    = 0x%" PRIx64 "\n", tag->lfb.fb_virt);
		kprintf("  fb_size    = 0x%" PRIx32 "\n", tag->lfb.fb_size);

		if(tag->lfb.flags & KBOOT_LFB_RGB) {
			kprintf("  red_size   = %" PRIu8 "\n", tag->lfb.red_size);
			kprintf("  red_pos    = %" PRIu8 "\n", tag->lfb.red_pos);
			kprintf("  green_size = %" PRIu8 "\n", tag->lfb.green_size);
			kprintf("  green_pos  = %" PRIu8 "\n", tag->lfb.green_pos);
			kprintf("  blue_size  = %" PRIu8 "\n", tag->lfb.blue_size);
			kprintf("  blue_pos   = %" PRIu8 "\n", tag->lfb.blue_pos);
		} else if(tag->lfb.flags & KBOOT_LFB_INDEXED) {
			kprintf("  palette (%" PRIu16 " entries):\n", tag->lfb.palette_size);
			for(uint16_t i = 0; i < tag->lfb.palette_size; i++) {
				kprintf("    r = %-3u, g = %-3u, b = %-3u\n",
					tag->lfb.palette[i].red, tag->lfb.palette[i].green,
					tag->lfb.palette[i].blue);
			}
		}

		break;
	default:
		kprintf("  type = %u (unknown)\n", tag->type);
		break;
	}
}

/** Print an IP address.
 * @param addr		Address to print.
 * @param flags		Behaviour flags. */
static void print_ip_addr(kboot_ip_addr_t *addr, uint32_t flags) {
	if(flags & KBOOT_NET_IPV6) {
		kprintf("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			addr->v6[0], addr->v6[1], addr->v6[2], addr->v6[3],
			addr->v6[4], addr->v6[5], addr->v6[6], addr->v6[7],
			addr->v6[8], addr->v6[9], addr->v6[10], addr->v6[11],
			addr->v6[12], addr->v6[13], addr->v6[14], addr->v6[15]);
	} else {
		kprintf("%u.%u.%u.%u\n", addr->v4[0], addr->v4[1], addr->v4[2], addr->v4[3]);
	}
}

/** Dump a boot device tag. */
static void dump_bootdev_tag(kboot_tag_bootdev_t *tag) {
	kprintf("KBOOT_TAG_BOOTDEV:\n");

	switch(tag->type) {
	case KBOOT_BOOTDEV_NONE:
		kprintf("  type = %" PRIu32 " (KBOOT_BOOTDEV_NONE)\n", tag->type);
		break;
	case KBOOT_BOOTDEV_DISK:
		kprintf("  type          = %" PRIu32 " (KBOOT_BOOTDEV_DISK)\n", tag->type);
		kprintf("  flags         = 0x%" PRIx32 "\n", tag->disk.flags);
		kprintf("  uuid          = `%s'\n", tag->disk.uuid);
		kprintf("  device        = 0x%x\n", tag->disk.device);
		kprintf("  partition     = 0x%x\n", tag->disk.partition);
		kprintf("  sub_partition = 0x%x\n", tag->disk.sub_partition);
		break;
	case KBOOT_BOOTDEV_NET:
		kprintf("  type        = %" PRIu32 " (KBOOT_BOOTDEV_NET)\n", tag->type);
		kprintf("  flags       = 0x%" PRIx32 "\n", tag->net.flags);
		if(tag->net.flags & KBOOT_NET_IPV6)
			kprintf("    KBOOT_NET_IPV6\n");
		kprintf("  server_ip   = "); print_ip_addr(&tag->net.server_ip, tag->net.flags);
		kprintf("  server_port = %" PRIu16 "\n", tag->net.server_port);
		kprintf("  gateway_ip  = "); print_ip_addr(&tag->net.gateway_ip, tag->net.flags);
		kprintf("  client_ip   = "); print_ip_addr(&tag->net.client_ip, tag->net.flags);
		kprintf("  client_mac  = %02x:%02x:%02x:%02x:%02x:%02x\n",
			tag->net.client_mac[0], tag->net.client_mac[1],
			tag->net.client_mac[2], tag->net.client_mac[3],
			tag->net.client_mac[4], tag->net.client_mac[5]);
		kprintf("  hw_addr_len = %u\n", tag->net.hw_addr_len);
		kprintf("  hw_type     = %u\n", tag->net.hw_type);
		break;
	default:
		kprintf("  type = %" PRIu32 " (unknown)\n", tag->type);
		break;
	}
}

/** Dump a log tag. */
static void dump_log_tag(kboot_tag_log_t *tag) {
	kboot_log_t *log;

	kprintf("KBOOT_TAG_LOG:\n");
	kprintf("  log_virt  = 0x%" PRIx64 "\n", tag->log_virt);
	kprintf("  log_phys  = 0x%" PRIx64 "\n", tag->log_phys);
	kprintf("  log_size  = %" PRIu32 "\n", tag->log_size);
	kprintf("  prev_phys = 0x%" PRIx64 "\n", tag->prev_phys);
	kprintf("  prev_size = %" PRIu32 "\n", tag->prev_size);

	log = (kboot_log_t *)((ptr_t)tag->log_virt);
	kprintf("  magic     = 0x%" PRIx32 "\n", log->magic);
}

/** Get a section by index.
 * @param tag		Tag to get from.
 * @param index		Index to get. */
static elf_shdr_t *find_elf_section(kboot_tag_sections_t *tag, uint32_t index) {
	return (elf_shdr_t *)&tag->sections[index * tag->entsize];
}

/** Dump a sections tag. */
static void dump_sections_tag(kboot_tag_sections_t *tag) {
	const char *strtab;
	elf_shdr_t *shdr;

	kprintf("KBOOT_TAG_SECTIONS:\n");
	kprintf("  num      = %" PRIu32 "\n", tag->num);
	kprintf("  entsize  = %" PRIu32 "\n", tag->entsize);
	kprintf("  shstrndx = %" PRIu32 "\n", tag->shstrndx);

	shdr = find_elf_section(tag, tag->shstrndx);
	strtab = (const char *)P2V(shdr->sh_addr);
	kprintf("  shstrtab = 0x%lx (%p)\n", shdr->sh_addr, strtab);

	for(uint32_t i = 0; i < tag->num; i++) {
		shdr = find_elf_section(tag, i);

		kprintf("  section %u (`%s'):\n", i, (shdr->sh_name) ? strtab + shdr->sh_name : "");
		kprintf("    sh_type  = %" PRIu32 "\n", shdr->sh_type);
		kprintf("    sh_flags = 0x%" PRIx32 "\n", shdr->sh_flags);
		kprintf("    sh_addr  = %p\n", shdr->sh_addr);
		kprintf("    sh_size  = %" PRIu32 "\n", shdr->sh_size);
	}
}

/** Get an E820 tag type. */
static const char *e820_tag_type(uint32_t type) {
	switch(type) {
	case 1:		return "Free";
	case 2:		return "Reserved";
	case 3:		return "ACPI reclaimable";
	case 4:		return "ACPI NVS";
	case 5:		return "Bad";
	case 6:		return "Disabled";
	default:	return "???";
	}
}

/** Dump an E820 tag. */
static void dump_e820_tag(kboot_tag_e820_t *tag) {
	kprintf("KBOOT_TAG_E820:\n");
	kprintf("  start  = 0x%" PRIx64 "\n", tag->start);
	kprintf("  length = 0x%" PRIx64 "\n", tag->length);
	kprintf("  type   = %" PRIu32 " (%s)\n", tag->type, e820_tag_type(tag->type));
}

/** Entry point of the test kernel.
 * @param magic		KBoot magic number.
 * @param tags		Tag list pointer. */
void kmain(uint32_t magic, kboot_tag_t *tags) {
	if(magic != KBOOT_MAGIC)
		while(true);

	console_init(tags);
	log_init(tags);

	kprintf("Test kernel loaded: magic: 0x%x, tags: %p\n", magic, tags);

	while(tags->type != KBOOT_TAG_NONE) {
		switch(tags->type) {
		case KBOOT_TAG_CORE:
			dump_core_tag((kboot_tag_core_t *)tags);
			break;
		case KBOOT_TAG_OPTION:
			dump_option_tag((kboot_tag_option_t *)tags);
			break;
		case KBOOT_TAG_MEMORY:
			dump_memory_tag((kboot_tag_memory_t *)tags);
			break;
		case KBOOT_TAG_VMEM:
			dump_vmem_tag((kboot_tag_vmem_t *)tags);
			break;
		case KBOOT_TAG_PAGETABLES:
			dump_pagetables_tag((kboot_tag_pagetables_t *)tags);
			break;
		case KBOOT_TAG_MODULE:
			dump_module_tag((kboot_tag_module_t *)tags);
			break;
		case KBOOT_TAG_VIDEO:
			dump_video_tag((kboot_tag_video_t *)tags);
			break;
		case KBOOT_TAG_BOOTDEV:
			dump_bootdev_tag((kboot_tag_bootdev_t *)tags);
			break;
		case KBOOT_TAG_LOG:
			dump_log_tag((kboot_tag_log_t *)tags);
			break;
		case KBOOT_TAG_SECTIONS:
			dump_sections_tag((kboot_tag_sections_t *)tags);
			break;
		case KBOOT_TAG_E820:
			dump_e820_tag((kboot_tag_e820_t *)tags);
			break;
		}

		tags = (kboot_tag_t *)ROUND_UP((ptr_t)tags + tags->size, 8);
	}

	kprintf("Tag list dump complete\n");

	#if defined(__i386__) || defined(__x86_64__)
	__asm__ volatile("wbinvd");
	#endif

	while(true);
}
