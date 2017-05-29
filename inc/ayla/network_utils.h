/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#ifndef LIB_AYLA_INCLUDE_AYLA_NETWORK_UTILS_H_
#define LIB_AYLA_INCLUDE_AYLA_NETWORK_UTILS_H_

#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>

/*
 * Recommeded max number of servers (RFC1537, section 7)
 */
#define NET_DNS_SERVER_NUM_MAX	7

/*
 * Structure used to store address and status info for a
 * single network interface.
 */
struct net_ifinfo {
	char name[IFNAMSIZ];	/* Interface name */
	uint32_t flags;			/* flags from SIOCGIFFLAGS */
	struct sockaddr_in addr;	/* address of interface */
	struct sockaddr_in netmask;	/* netmask of interface */
	struct ether_addr hw_addr;	/* HW address */
};

/*
 * Struct used to store IP addresses of DNS servers
 */
struct net_dnsservers {
	size_t num;
	struct sockaddr_in addrs[NET_DNS_SERVER_NUM_MAX];
};

/*
 * The data field size in struct sockaddr varies by address family.
 * Return the data size in bytes based on the address type.
 */
size_t net_get_addr_data_size(const struct sockaddr *addr);

/*
 * The data fields in struct sockaddr vary by address family.
 * Return a pointer to the address data field based on the address type.
 */
unsigned char *net_get_addr_data(const struct sockaddr *addr);

/*
 * Queries the system for address and status info on a single
 * network interface.  Returns 0 if info struct successfully
 * populated, or -1 if an error occurred.
 */
int net_get_ifinfo(const char *ifname, struct net_ifinfo *info);

/*
 * Read nameservers from /etc/resolv.conf
 */
int net_get_dnsservers(struct net_dnsservers *servers);

/*
 * Check all interfaces in the supplied address family, and
 * return non-zero if address is on the local subnet.
 */
int net_is_local_addr(const struct sockaddr *test_addr);

/*
 * Alternative to ether_ntoa() that guarantees two digit 0-padded hex
 * bytes.
 */
char *net_ether_to_str(const struct ether_addr *addr);

#endif /* LIB_AYLA_INCLUDE_AYLA_NETWORK_UTILS_H_ */
