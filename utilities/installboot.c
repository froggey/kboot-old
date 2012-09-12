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
 * @brief		Ext2/3/4 boot sector installation utility.
 */

#include <sys/stat.h>

#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

/** Boot sector data structure. */
struct __attribute__((packed)) bootsect {
	uint8_t data1[502];
	uint32_t partition_lba;
	uint32_t inode;
	uint8_t data2[514];
};

int main(int argc, char **argv) {
	struct bootsect sect;
	int ofd, ifd;
	ssize_t ret;
	off_t offset;
	struct stat st;
	void *buf;

	if(argc != 5) {
		printf("Usage: %s <image> <boot sector> <partition start LBA> <inode number>\n", argv[0]);
		printf("Don't use this unless you really really know what you are doing!\n");
		printf("A proper utility will be implemented soon.\n");
		return 1;
	}

	ofd = open(argv[1], O_WRONLY);
	if(ofd < 0) {
		perror("open");
		return 1;
	}

	ifd = open(argv[2], O_RDONLY);
	if(ifd < 0) {
		perror("open");
		return 1;
	}

	fstat(ifd, &st);
	if(st.st_size != sizeof(sect)) {
		printf("Incorrect boot sector size\n");
		return 1;
	}

	ret = read(ifd, &sect, sizeof(sect));
	if(ret != sizeof(sect)) {
		perror("read");
		return 1;
	}
	close(ifd);

	errno = 0;
	sect.partition_lba = strtoul(argv[3], NULL, 0);
	if(errno != 0) {
		perror("strtoul");
		return 1;
	}

	errno = 0;
	sect.inode = strtoul(argv[4], NULL, 0);
	if(errno != 0) {
		perror("strtoul");
		return 1;
	}

	// FIXME: Sector size independence (stat does NOT give device block size)
	offset = (off_t)sect.partition_lba * 512;
	printf("Writing to block %" PRIu32 ", offset %" PRId64 "\n", sect.partition_lba, offset);
	ret = pwrite(ofd, &sect, sizeof(sect), offset);
	if(ret != sizeof(sect)) {
		perror("pwrite");
		return 1;
	}
	close(ofd);

	return 0;
}