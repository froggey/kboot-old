/*
 * Copyright (C) 2009-2012 Alex Smith
 * Copyright (C) 2012 Daniel Collins
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
 * @brief		MSDOS partition table scanner.
 *
 * @fixme		Endianness.
 */

#include <lib/utility.h>

#include <loader.h>
#include <memory.h>

#include "msdos.h"

/** Check whether a partition is valid.
 * @param disk		Disk containing partition record.
 * @param part		Partition record.
 * @return		Whether the partition is valid. */
static bool is_valid_partition(disk_t *disk, msdos_part_t *part) {
	if(part->type == 0 || (part->bootable != 0 && part->bootable != 0x80)) {
		return false;
	} else if(part->start_lba >= disk->blocks) {
		return false;
	} else if(part->start_lba + part->num_sects > disk->blocks) {
		return false;
	}

	return true;
}

/** Check whether a partition is an extended partition.
 * @param part		Partition record.
 * @return		Whether the partition is extended. */
static bool is_extended_partition(msdos_part_t *part) {
	switch(part->type) {
	case 0x5:
	case 0xF:
	case 0x85:
		/* These are different types of extended partition, 0x5 is
		 * supposedly with CHS addressing, while 0xF is LBA. However,
		 * Linux treats them the exact same way. */
		return true;
	default:
		return false;
	}
}

/** Iterate over an extended partition.
 * @param disk		Disk that the partition is on.
 * @param offset	Offset of the extended partition.
 * @param size		Size of the extended partition.
 * @param cb		Callback function. */
static void extended_partition_iterate(disk_t *disk, uint32_t offset, uint32_t size,
	partition_map_iterate_cb_t cb)
{
	msdos_mbr_t *ebr = kmalloc(sizeof(msdos_mbr_t));
	uint32_t curr_ebr, next_ebr;
	msdos_part_t *part, *next;
	size_t idx = 4;

	curr_ebr = offset;

	do {
		if(!disk_read(disk, ebr, sizeof(*ebr), (uint64_t)curr_ebr * disk->block_size)) {
			dprintf("disk: failed to read EBR at %u\n", curr_ebr);
			break;
		} else if(ebr->signature != MSDOS_SIGNATURE) {
			dprintf("disk: warning: invalid EBR, corrupt partition table\n");
			break;
		}

		/* First entry contains the logical partition, second entry
		 * refers to the next EBR, forming a linked list of EBRs. */
		part = &ebr->partitions[0];
		next = &ebr->partitions[1];

		/* Calculate next_ebr. The start sector is relative to the start
		 * of the extended partition. Set to 0 if the second partition
		 * doesn't refer to another EBR entry, causes the loop to end. */
		next_ebr = (next->start_lba += offset);
		if(!is_valid_partition(disk, next) || !is_extended_partition(next) || next_ebr <= curr_ebr)
			next_ebr = 0;

		/* Get the partition. Here the start sector is relative to the
		 * current EBR's location. */
		part->start_lba += curr_ebr;
		if(!is_valid_partition(disk, part))
			continue;

		dprintf("disk: found logical MSDOS partition %d:\n", idx);
		dprintf(" type:      0x%x\n", part->type);
		dprintf(" start_lba: %u\n", part->start_lba);
		dprintf(" num_sects: %u\n", part->num_sects);

		cb(disk, idx++, part->start_lba, part->num_sects);
	} while((curr_ebr = next_ebr));

	kfree(ebr);
}

/** Iterate over the partitions on a device.
 * @param disk		Disk to iterate over.
 * @param cb		Callback function.
 * @return		Whether the device contained an MSDOS partition table. */
static bool msdos_partition_iterate(disk_t *disk, partition_map_iterate_cb_t cb) {
	msdos_mbr_t *mbr = kmalloc(sizeof(msdos_mbr_t));
	bool seen_extended = false;
	msdos_part_t *part;
	size_t i;

	/* Read in the MBR, which is in the first block on the device. */
	if(!disk_read(disk, mbr, sizeof(*mbr), 0) || mbr->signature != MSDOS_SIGNATURE) {
		kfree(mbr);
		return false;
	}

	/* Loop through all partitions in the table. */
	for(i = 0; i < 4; i++) {
		part = &mbr->partitions[i];

		if(!is_valid_partition(disk, part))
			continue;

		if(is_extended_partition(part)) {
			if(seen_extended) {
				dprintf("disk: warning: ignoring multiple extended partitions...\n");
				continue;
			}

			extended_partition_iterate(disk, part->start_lba, part->num_sects, cb);
			seen_extended = true;
		} else {
			dprintf("disk: found MSDOS partition %d:\n", i);
			dprintf(" type:      0x%x\n", part->type);
			dprintf(" start_lba: %u\n", part->start_lba);
			dprintf(" num_sects: %u\n", part->num_sects);

			cb(disk, i, part->start_lba, part->num_sects);
		}
	}

	kfree(mbr);
	return true;
}

/** MS-DOS partition map type. */
BUILTIN_PARTITION_MAP(msdos_partition_map) = {
	.iterate = msdos_partition_iterate,
};
