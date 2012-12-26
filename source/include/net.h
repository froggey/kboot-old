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
 * @brief		Network boot definitions.
 */

#ifndef __NET_H
#define __NET_H

#include <device.h>

/** Type used to store a MAC address. */
typedef uint8_t mac_addr_t[8];

/** Type used to store an IPv4 address. */
typedef uint8_t ipv4_addr_t[4];

/** Type used to store an IPv6 address. */
typedef uint8_t ipv6_addr_t[16];

/** Type used to store an IP address. */
typedef union ip_addr {
	ipv4_addr_t v4;			/**< IPv4 address. */
	ipv6_addr_t v6;			/**< IPv6 address. */
} ip_addr_t;

/** Structure containing details of a network boot server. */
typedef struct net_device {
	device_t device;		/**< Device header. */

	uint32_t flags;			/**< Behaviour flags. */
	ip_addr_t server_ip;		/**< Server IP address. */
	uint16_t server_port;		/**< UDP port number of TFTP server. */
	ip_addr_t gateway_ip;		/**< Gateway IP address. */
	ip_addr_t client_ip;		/**< IP used on this machine when communicating with server. */
	mac_addr_t client_mac;		/**< MAC address of the boot network interface. */
} net_device_t;

/** Network device flags. */
#define NET_DEVICE_IPV6		(1<<0)	/**< Device is configured using IPv6. */

#endif /* __NET_H */
