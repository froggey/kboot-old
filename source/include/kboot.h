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
 * @brief		KBoot boot protocol definitions.
 */

#ifndef __KBOOT_H
#define __KBOOT_H

/** Magic number passed to the entry point of a KBoot kernel. */
#define KBOOT_MAGIC			0xB007CAFE

/** Current KBoot version. */
#define KBOOT_VERSION			1

#ifndef __ASM__

#include <types.h>

/** Type used to store a physical address. */
typedef uint64_t kboot_paddr_t;

/** Type used to store a virtual address. */
typedef uint64_t kboot_vaddr_t;

/**
 * Information tags.
 */

/** KBoot information tag header structure. */
typedef struct kboot_tag {
	uint32_t type;				/**< Type of the tag. */
	uint32_t size;				/**< Total size of the tag data. */
} kboot_tag_t;

/** Possible information tag types. */
#define KBOOT_TAG_NONE			0	/**< End of tag list. */
#define KBOOT_TAG_CORE			1	/**< Core information tag (always present). */
#define KBOOT_TAG_OPTION		2	/**< Kernel option. */
#define KBOOT_TAG_MEMORY		3	/**< Physical memory range. */
#define KBOOT_TAG_VMEM			4	/**< Virtual memory range. */
#define KBOOT_TAG_PAGETABLES		5	/**< Page table information (architecture-specific). */
#define KBOOT_TAG_MODULE		6	/**< Boot module. */
#define KBOOT_TAG_VIDEO			7	/**< Video mode information. */
#define KBOOT_TAG_BOOTDEV		8	/**< Boot device information. */
#define KBOOT_TAG_LOG			9	/**< Kernel log buffer. */
#define KBOOT_TAG_SECTIONS		10	/**< ELF section information. */
#define KBOOT_TAG_E820			11	/**< BIOS address range descriptor (PC-specific). */

/** Tag containing core information for the kernel. */
typedef struct kboot_tag_core {
	kboot_tag_t header;			/**< Tag header. */

	kboot_paddr_t tags_phys;		/**< Physical address of the tag list. */
	uint32_t tags_size;			/**< Total size of the tag list (rounded to 8 bytes). */
	uint32_t _pad;

	kboot_paddr_t kernel_phys;		/**< Physical address of the kernel image. */

	kboot_vaddr_t stack_base;		/**< Virtual address of the boot stack. */
	kboot_paddr_t stack_phys;		/**< Physical address of the boot stack. */
	uint32_t stack_size;			/**< Size of the boot stack. */
} kboot_tag_core_t;

/** Tag containing an option passed to the kernel. */
typedef struct kboot_tag_option {
	kboot_tag_t header;			/**< Tag header. */

	uint8_t type;				/**< Type of the option. */
	uint32_t name_size;			/**< Length of name string, including null terminator. */
	uint32_t value_size;			/**< Size of the option value, in bytes. */
} kboot_tag_option_t;

/** Possible option types. */
#define KBOOT_OPTION_BOOLEAN		0	/**< Boolean. */
#define KBOOT_OPTION_STRING		1	/**< String. */
#define KBOOT_OPTION_INTEGER		2	/**< Integer. */

/** Tag describing a physical memory range. */
typedef struct kboot_tag_memory {
	kboot_tag_t header;			/**< Tag header. */

	kboot_paddr_t start;			/**< Start of the memory range. */
	kboot_paddr_t size;			/**< Size of the memory range. */
	uint8_t type;				/**< Type of the memory range. */
} kboot_tag_memory_t;

/** Possible memory range types. */
#define KBOOT_MEMORY_FREE		0	/**< Free, usable memory. */
#define KBOOT_MEMORY_ALLOCATED		1	/**< Allocated memory. */
#define KBOOT_MEMORY_RECLAIMABLE	2	/**< Memory reclaimable when boot information is no longer needed. */

/** Tag describing a virtual memory range. */
typedef struct kboot_tag_vmem {
	kboot_tag_t header;			/**< Tag header. */

	kboot_vaddr_t start;			/**< Start of the virtual memory range. */
	kboot_vaddr_t size;			/**< Size of the virtual memory range. */
	kboot_paddr_t phys;			/**< Physical address that this range maps to. */
} kboot_tag_vmem_t;

/** Tag describing a boot module. */
typedef struct kboot_tag_module {
	kboot_tag_t header;			/**< Tag header. */

	kboot_paddr_t addr;			/**< Address of the module. */
	uint32_t size;				/**< Size of the module. */
} kboot_tag_module_t;

/** Structure describing an RGB colour. */
typedef struct kboot_colour {
	uint8_t red;				/**< Red value. */
	uint8_t green;				/**< Green value. */
	uint8_t blue;				/**< Blue value. */
} kboot_colour_t;

/** Tag containing video mode information. */
typedef struct kboot_tag_video {
	kboot_tag_t header;			/**< Tag header. */

	uint32_t type;				/**< Type of the video mode set up. */
	uint32_t _pad;

	union {
		/** VGA text mode information. */
		struct {
			uint8_t cols;		/**< Columns on the text display. */
			uint8_t lines;		/**< Lines on the text display. */
			uint8_t x;		/**< Cursor X position. */
			uint8_t y;		/**< Cursor Y position. */
			uint32_t _pad;
			kboot_paddr_t mem_phys;	/**< Physical address of VGA memory. */
			kboot_vaddr_t mem_virt;	/**< Virtual address of VGA memory. */
			uint32_t mem_size;	/**< Size of VGA memory mapping. */
		} vga;

		/** Linear framebuffer mode information. */
		struct {
			uint32_t flags;		/**< LFB properties. */
			uint32_t width;		/**< Width of video mode, in pixels. */
			uint32_t height;	/**< Height of video mode, in pixels. */
			uint8_t bpp;		/**< Number of bits per pixel. */
			uint32_t pitch;		/**< Number of bytes per line of the framebuffer. */
			uint32_t _pad;
			kboot_paddr_t fb_phys;	/**< Physical address of the framebuffer. */
			kboot_vaddr_t fb_virt;	/**< Virtual address of a mapping of the framebuffer. */
			uint32_t fb_size;	/**< Size of the virtual mapping. */
			uint8_t red_size;	/**< Size of red component of each pixel. */
			uint8_t red_pos;	/**< Bit position of the red component of each pixel. */
			uint8_t green_size;	/**< Size of green component of each pixel. */
			uint8_t green_pos;	/**< Bit position of the green component of each pixel. */
			uint8_t blue_size;	/**< Size of blue component of each pixel. */
			uint8_t blue_pos;	/**< Bit position of the blue component of each pixel. */
			uint16_t palette_size;	/**< For indexed modes, size of the colour palette. */

			/** For indexed modes, the colour palette set by the loader. */
			kboot_colour_t palette[0];
		} lfb;
	};
} kboot_tag_video_t;

/** Video mode types. */
#define KBOOT_VIDEO_VGA			(1<<0)	/**< VGA text mode. */
#define KBOOT_VIDEO_LFB			(1<<1)	/**< Linear framebuffer. */

/** Linear framebuffer flags. */
#define KBOOT_LFB_RGB			(1<<0)	/**< Direct RGB colour format. */
#define KBOOT_LFB_INDEXED		(1<<1)	/**< Indexed colour format. */

/** Type used to store a MAC address. */
typedef uint8_t kboot_mac_addr_t[8];

/** Type used to store an IPv4 address. */
typedef uint8_t kboot_ipv4_addr_t[4];

/** Type used to store an IPv6 address. */
typedef uint8_t kboot_ipv6_addr_t[16];

/** Type used to store an IP address. */
typedef union kboot_ip_addr {
	kboot_ipv4_addr_t v4;			/**< IPv4 address. */
	kboot_ipv6_addr_t v6;			/**< IPv6 address. */
} kboot_ip_addr_t;

/** Tag containing boot device information. */
typedef struct kboot_tag_bootdev {
	kboot_tag_t header;			/**< Tag header. */

	uint32_t method;			/**< Method used to boot. */

	union {
		/** Disk device information. */
		struct {
			uint32_t flags;		/**< Behaviour flags. */
			uint8_t uuid[64];	/**< UUID of the boot filesystem. */
			uint8_t device;		/**< Device ID (platform-specific). */
			uint8_t partition;	/**< Partition number. */
			uint8_t sub_partition;	/**< Sub-partition number. */
		} disk;

		/** Network boot information. */
		struct {
			uint32_t flags;		/**< Behaviour flags. */

			/** Server IP address. */
			kboot_ip_addr_t server_ip;

			/** UDP port number of TFTP server. */
			uint16_t server_port;

			/** Gateway IP address. */
			kboot_ip_addr_t gateway_ip;

			/** IP used on this machine when communicating with server. */
			kboot_ip_addr_t client_ip;

			/** MAC address of the boot network interface. */
			kboot_mac_addr_t client_mac;
		} net;
	};
} kboot_tag_bootdev_t;

/** Boot method types. */
#define KBOOT_METHOD_NONE		0	/**< No boot device (e.g. boot image). */
#define KBOOT_METHOD_DISK		1	/**< Booted from a disk device. */
#define KBOOT_METHOD_NET		2	/**< Booted from the network. */

/** Network boot behaviour flags. */
#define KBOOT_NET_IPV6			(1<<0)	/**< Given addresses are IPv6 addresses. */

/** Tag describing the kernel log buffer. */
typedef struct kboot_tag_log {
	kboot_tag_t header;			/**< Tag header. */

	kboot_vaddr_t log_virt;			/**< Virtual address of log buffer. */
	kboot_paddr_t log_phys;			/**< Physical address of log buffer. */
	uint32_t log_size;			/**< Size of log buffer. */
	uint32_t _pad;

	kboot_paddr_t prev_phys;		/**< Physical address of previous log buffer. */
	uint32_t prev_size;			/**< Size of previous log buffer. */
} kboot_tag_log_t;

/** Structure of a log buffer. */
typedef struct kboot_log {
	uint32_t magic;				/**< Magic value used by loader (should not be overwritten). */

	uint32_t start;				/**< Offset in the buffer of the start of the log. */
	uint32_t length;			/**< Number of charcters in the log buffer. */

	uint32_t info[3];			/**< Fields for use by the kernel. */
	uint8_t buffer[0];			/**< Log data. */
} kboot_log_t;

/** Tag describing ELF section headers. */
typedef struct kboot_tag_sections {
	kboot_tag_t header;			/**< Tag header. */

	uint32_t num;				/**< Number of section headers. */
	uint32_t entsize;			/**< Size of each section header. */
	uint32_t shstrndx;			/**< Section name string table index. */

	uint32_t _pad;

	uint8_t sections[0];			/**< Section data. */
} kboot_tag_sections_t;

/** Tag containing an E820 address range descriptor (PC-specific). */
typedef struct kboot_tag_e820 {
	kboot_tag_t header;			/**< Tag header. */

	uint64_t start;
	uint64_t length;
	uint32_t type;
	uint32_t attr;
} kboot_tag_e820_t;

#if defined(__x86_64__) || defined(__i386__)
/** Tag containing page table information. */
typedef struct kboot_tag_pagetables {
	kboot_tag_t header;			/**< Tag header. */

#if defined(__x86_64__)
	kboot_paddr_t pml4;			/**< Physical address of the PML4. */
#else
	kboot_paddr_t page_dir;			/**< Physical address of the page directory. */
#endif
	kboot_vaddr_t mapping;			/**< Virtual address of recursive mapping. */
} kboot_tag_pagetables_t;
#endif

/**
 * Image tags.
 */

/** KBoot ELF note name. */
#define KBOOT_NOTE_NAME			"KBoot"

/** Power-of-2 alignment of note fields (ELF32 requires 4 bytes, ELF64 requires 8). */
#ifdef __LP64__
# define KBOOT_NOTE_ALIGN		3
#else
# define KBOOT_NOTE_ALIGN		2
#endif

/** KBoot image tag types (used as ELF note type field). */
#define KBOOT_ITAG_IMAGE		0	/**< Basic image information (required). */
#define KBOOT_ITAG_LOAD			1	/**< Memory layout options. */
#define KBOOT_ITAG_OPTION		2	/**< Option description. */
#define KBOOT_ITAG_MAPPING		3	/**< Virtual memory mapping description. */
#define KBOOT_ITAG_VIDEO		4	/**< Requested video mode. */

/** Image tag containing basic image information. */
typedef struct kboot_itag_image {
	uint32_t version;			/**< KBoot version that the image is using. */
	uint32_t flags;				/**< Flags for the image. */
} kboot_itag_image_t;

/** Flags controlling optional features. */
#define KBOOT_IMAGE_SECTIONS		(1<<0)	/**< Load ELF sections and pass a sections tag. */
#define KBOOT_IMAGE_LOG			(1<<1)	/**< Enable the kernel log facility. */

/** Macro to declare an image itag. */
#define KBOOT_IMAGE(flags) \
	__asm__( \
		"   .pushsection \".note.kboot.image\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 3f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_IMAGE) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .long " XSTRINGIFY(KBOOT_VERSION) "\n" \
		"   .long " STRINGIFY(flags) "\n" \
		"3: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Image tag specifying loading parameters. */
typedef struct kboot_itag_load {
	uint32_t flags;				/**< Flags controlling load behaviour. */
	uint32_t _pad;
	kboot_paddr_t alignment;		/**< Requested physical alignment of kernel image. */
	kboot_paddr_t min_alignment;		/**< Minimum physical alignment of kernel image. */
	kboot_paddr_t phys_address;		/**< If KBOOT_LOAD_FIXED is set, address to load at. */
	kboot_vaddr_t virt_map_base;		/**< Base of virtual mapping range. */
	kboot_vaddr_t virt_map_size;		/**< Size of virtual mapping range. */
} kboot_itag_load_t;

/** Flags controlling load behaviour. */
#define KBOOT_LOAD_FIXED		(1<<0)	/**< Load at a fixed physical address. */

/** Macro to declare a load itag. */
#define KBOOT_LOAD(flags, alignment, min_alignment, phys_address, virt_map_base, virt_map_size) \
	__asm__( \
		"   .pushsection \".note.kboot.load\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 3f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_LOAD) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .long " STRINGIFY(flags) "\n" \
		"   .long 0\n" \
		"   .long " STRINGIFY(alignment) "\n" \
		"   .long " STRINGIFY(min_alignment) "\n" \
		"   .long " STRINGIFY(phys_address) "\n" \
		"   .long " STRINGIFY(virt_map_base) "\n" \
		"   .long " STRINGIFY(virt_map_size) "\n" \
		"3: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Image tag containing an option description. */
typedef struct kboot_itag_option {
	uint8_t type;				/**< Type of the option. */
	uint32_t name_len;			/**< Length of the option name. */
	uint32_t desc_len;			/**< Length of the option description. */
	uint32_t default_len;			/**< Length of the default value. */
} kboot_itag_option_t;

/** Macro to declare a boolean option itag. */
#define KBOOT_BOOLEAN_OPTION(name, desc, default) \
	__asm__( \
		"   .pushsection \".note.kboot.option." name "\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 6f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_OPTION) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .byte " XSTRINGIFY(KBOOT_OPTION_BOOLEAN) "\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .long 4f - 3f\n" \
		"   .long 5f - 4f\n" \
		"   .long 1\n" \
		"3: .asciz \"" name "\"\n" \
		"4: .asciz \"" desc "\"\n" \
		"5: .byte " STRINGIFY(default) "\n" \
		"6: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Macro to declare an integer option itag. */
#define KBOOT_INTEGER_OPTION(name, desc, default) \
	__asm__( \
		"   .pushsection \".note.kboot.option." name "\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 6f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_OPTION) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .byte " XSTRINGIFY(KBOOT_OPTION_INTEGER) "\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .long 4f - 3f\n" \
		"   .long 5f - 4f\n" \
		"   .long 8\n" \
		"3: .asciz \"" name "\"\n" \
		"4: .asciz \"" desc "\"\n" \
		"5: .quad " STRINGIFY(default) "\n" \
		"6: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Macro to declare an string option itag. */
#define KBOOT_STRING_OPTION(name, desc, default) \
	__asm__( \
		"   .pushsection \".note.kboot.option." name "\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 6f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_OPTION) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .byte " XSTRINGIFY(KBOOT_OPTION_STRING) "\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .byte 0\n" \
		"   .long 4f - 3f\n" \
		"   .long 5f - 4f\n" \
		"   .long 6f - 5f\n" \
		"3: .asciz \"" name "\"\n" \
		"4: .asciz \"" desc "\"\n" \
		"5: .asciz \"" default "\"\n" \
		"6: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Image tag containing a virtual memory mapping description. */
typedef struct kboot_itag_mapping {
	kboot_vaddr_t virt;			/**< Virtual address to map. */
	kboot_paddr_t phys;			/**< Physical address to map to. */
	kboot_vaddr_t size;			/**< Size of mapping to make. */
} kboot_itag_mapping_t;

/** Macro to declare a virtual memory mapping itag. */
#define KBOOT_MAPPING(virt, phys, size) \
	__asm__( \
		"   .pushsection \".note.kboot.mapping.b" STRINGIFY(virt) "\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 3f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_MAPPING) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .quad " STRINGIFY(virt) "\n" \
		"   .quad " STRINGIFY(phys) "\n" \
		"   .quad " STRINGIFY(size) "\n" \
		"3: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

/** Image tag specifying the kernel's requested video mode. */
typedef struct kboot_itag_video {
	uint32_t types;				/**< Supported video mode types. */
	uint32_t width;				/**< Preferred LFB width. */
	uint32_t height;			/**< Preferred LFB height. */
	uint8_t bpp;				/**< Preferred LFB bits per pixel. */
} kboot_itag_video_t;

/** Macro to declare a video mode itag. */
#define KBOOT_VIDEO(types, width, height, bpp) \
	__asm__( \
		"   .pushsection \".note.kboot.video\", \"a\"\n" \
		"   .long 1f - 0f\n" \
		"   .long 3f - 2f\n" \
		"   .long " XSTRINGIFY(KBOOT_ITAG_MAPPING) "\n" \
		"0: .asciz \"KBoot\"\n" \
		"1: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"2: .long " STRINGIFY(types) "\n" \
		"   .long " STRINGIFY(width) "\n" \
		"   .long " STRINGIFY(height) "\n" \
		"   .byte " STRINGIFY(bpp) "\n" \
		"3: .p2align " XSTRINGIFY(KBOOT_NOTE_ALIGN) "\n" \
		"   .popsection\n")

#endif /* __ASM__ */
#endif /* __KBOOT_H */
