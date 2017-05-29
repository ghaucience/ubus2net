/*
 * Copyright 2011-2017 Ayla Networks, Inc.  All rights reserved.
 *
 * Use of the accompanying software is permitted only in accordance
 * with and subject to the terms of the Software License Agreement
 * with Ayla Networks, Inc., a copy of which can be obtained from
 * Ayla Networks, Inc.
 */

#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <ayla/utypes.h>
#include <ayla/hex.h>
#include <ayla/log.h>
#include <ayla/network_utils.h>


/*
 * The data field size in struct sockaddr varies by address family.
 * Return the data size in bytes based on the address type.
 */
size_t net_get_addr_data_size(const struct sockaddr *addr)
{
	return addr->sa_family == AF_INET ?
	    sizeof(struct in_addr) : sizeof(struct in6_addr);
}

/*
 * The data fields in struct sockaddr vary by address family.
 * Return a pointer to the address data field based on the address type.
 */
unsigned char *net_get_addr_data(const struct sockaddr *addr)
{
	struct sockaddr_in *sa;
	struct sockaddr_in6 *sa6;

	switch (addr->sa_family) {
	case AF_INET:
		sa = (struct sockaddr_in *)addr;
		return (unsigned char *)&sa->sin_addr.s_addr;
	case AF_INET6:
		sa6 = (struct sockaddr_in6 *)addr;
		return sa6->sin6_addr.s6_addr;
	default:
		return NULL;
	}
}

/*
 * Queries the system for address and status info on a single
 * network interface.  Returns 0 if info struct successfully
 * populated, or -1 if an error occurred.
 */
int net_get_ifinfo(const char *ifname, struct net_ifinfo *info)
{
	int fd = -1;
	int err = -1;
	struct ifreq ifr;
	struct sockaddr_in *addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("socket open failed: %m");
		goto error;
	}

	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);


	/* get flags */
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		log_err("ioctl SIOCGIFFLAGS failed: %m");
		goto error;
	}
	info->flags = ifr.ifr_flags;

	/* get HW address */
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		log_err("ioctl SIOCGIFHWADDR failed: %m");
		goto error;
	}
	memcpy(&info->hw_addr,
	    ((struct sockaddr *)&ifr.ifr_hwaddr)->sa_data,
	    sizeof(info->hw_addr));

	if (info->flags & IFF_RUNNING) {
		ifr.ifr_addr.sa_family = AF_INET;	/* get IPv4 address */
		addr = (struct sockaddr_in *)&ifr.ifr_addr;
		/* get address (AF_INET only) */
		if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
			log_err("ioctl SIOCGIFADDR failed: %m");
			goto error;
		}
		memcpy(&info->addr, addr, sizeof(info->addr));

		/* get netmask (AF_INET only) */
		if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
			log_err("ioctl SIOCGIFNETMASK failed: %m");
			goto error;
		}
		memcpy(&info->netmask, addr, sizeof(info->netmask));
	} else {
		memset(&info->addr, 0, sizeof(info->addr));
		memset(&info->netmask, 0, sizeof(info->netmask));
	}

	/* set interface name */
	snprintf(info->name, sizeof(info->name), "%s", ifname);

	err = 0;
error:
	if (fd >= 0) {
		close(fd);
	}
	return err;
}

/*
 * Read nameservers from /etc/resolv.conf
 */
int net_get_dnsservers(struct net_dnsservers *servers)
{
	FILE *fp;
	char line[120];	/* should be enough space for a line of resolv.conf */
	char *cp;
	struct sockaddr_in *addr;

	fp = fopen("/etc/resolv.conf", "r");
	if (!fp) {
		log_err("failed to open resolv.conf");
		return -1;
	}

	servers->num = 0;
	while (fgets(line, sizeof(line), fp)) {
		/* shortcut to ignore file comments */
		if (line[0] == '#') {
			continue;
		}
		cp = line;
		if (!strsep(&cp, " ") || !cp || !cp[0]) {
			continue;
		}
		if (strncmp(line, "nameserver", sizeof(line))) {
			continue;
		}
		if (servers->num >= NET_DNS_SERVER_NUM_MAX) {
			log_debug("exceeded %d DNS server max",
			    NET_DNS_SERVER_NUM_MAX);
			break;
		}
		addr = servers->addrs + servers->num;
		if (!inet_aton(cp, &addr->sin_addr)) {
			log_debug("invalid address: %s", cp);
			continue;
		}
		addr->sin_family = AF_INET;
		addr->sin_port = 0;
		++servers->num;
	}

	fclose(fp);
	return 0;
}

/*
 * Check all interfaces with the supplied type of address, and
 * return non-zero if addr is on the local subnet.
 */
int net_is_local_addr(const struct sockaddr *test_addr)
{
	int is_local = 0;
	struct ifaddrs *ifaddr, *ifa;
	unsigned char *test_data, *addr_data, *mask_data;
	size_t addr_size;
	int i;

	/* get a data structure with info on all network interfaces */
	if (getifaddrs(&ifaddr) == -1) {
		log_err("getifaddrs failed\n");
		return is_local;
	}

	test_data = net_get_addr_data(test_addr);

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		/* only check valid interfaces in the same IF family */
		if (!ifa->ifa_addr ||
		    ifa->ifa_addr->sa_family != test_addr->sa_family) {
			continue;
		}

		addr_data = net_get_addr_data(ifa->ifa_addr);
		mask_data = net_get_addr_data(ifa->ifa_netmask);
		addr_size = net_get_addr_data_size(ifa->ifa_addr);

		/* match addr's subnet */
		for (i = 0; i < addr_size; ++i) {
			if ((addr_data[i] & mask_data[i]) !=
			    (test_data[i] & mask_data[i])) {
				break;
			}
		}
		if (i == addr_size) {
			is_local = 1;
			break;
		}
	}
	freeifaddrs(ifaddr);
	return is_local;
}

/*
 * Alternative to ether_ntoa() that guarantees two digit 0-padded hex
 * bytes.
 */
char *net_ether_to_str(const struct ether_addr *addr)
{
	static char buf[ETH_ALEN * 3];

	if (hex_string(buf, sizeof(buf),
	    addr->ether_addr_octet, ETH_ALEN, false, ':') < 0) {
		return NULL;
	}
	return buf;
}
