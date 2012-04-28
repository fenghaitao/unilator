#include <bios/netdev.h>
#include <bios/buflist.h>
#include <bios/udp.h>
#include <bios/string.h>
#include <bios/time.h>
#include <bios/bootp.h>
#include <bios/stdio.h>
#include <net/if_arp.h>

#define CONF_RETRIES		10
#define CONF_TIMEOUT_BASE	100
#define CONF_TIMEOUT_MULT	2
#define CONF_TIMEOUT_MAX	6000

static	int ic_got_reply;	/* got a reply */
static  u32 ic_bootp_xid;

struct bootp_pkt	 ic_bootp;
struct netdev		*ic_netdev;

static int bootp_build(struct netdev *nd, struct bootp_pkt *p, int seconds)
{
	memset(p, 0, sizeof(*p));

	p->op	 = 1;
	p->htype = ARPHRD_ETHER;
	p->hlen	 = nd->hw_addr_len;
	p->xid	 = ic_bootp_xid;
	p->secs	 = htonl(seconds);

	memcpy(p->hw_addr, nd->hw_addr, nd->hw_addr_len);

	return sizeof(*p);
}

static void bootp_print_config(void)
{
	struct netdev *nd;

	for (nd = probed_net_devs; nd; nd = nd->next)
		if (nd->up)
			printf("%s: hardware address %02X:%02X:%02X:%02X:%02X:%02X\n",
				nd->name,
				nd->hw_addr[0],
				nd->hw_addr[1],
				nd->hw_addr[2],
				nd->hw_addr[3],
				nd->hw_addr[4],
				nd->hw_addr[5]);
}

static int bootp_send(int seconds)
{
	struct netdev *nd;
	struct bootp_pkt bootp;
	struct sin from, to;
	int nodev = -1;

	from.sin_port = htons(0x44);
	from.sin_addr = 0;

	to.sin_port = htons(0x43);
	to.sin_addr = INADDR_ANY;

	for (nd = probed_net_devs; nd; nd = nd->next) {
		if (nd->up) {
			struct buflist bl;

			bl.size = bootp_build(nd, &bootp, seconds);
			bl.data = &bootp;
			bl.next = NULL;

			if (udp_send(nd, &from, &to, &bl))
				nd->hard->close(nd);
			else
				nodev = 0;
		}
	}

	return nodev;
}

static int bootp_recv(void)
{
	struct netdev *nd;
	struct sin from, to;
	int nodev = -1;

	from.sin_port = 0;
	from.sin_addr = INADDR_ANY;

	to.sin_port = htons(0x44);
	to.sin_addr = INADDR_ANY;

	for (nd = probed_net_devs; nd; nd = nd->next) {
		if (nd->up) {
			int bytes;

			bytes = udp_recv(nd, &from, &to, &ic_bootp, sizeof(ic_bootp));

			if (bytes < 0) {
				nd->hard->close(nd);
				continue;
			} else
				nodev = 0;

			if (bytes < 300 || ic_bootp.op != 2 || ic_bootp.xid != ic_bootp_xid) {
				if (bytes)
					printf("?");
			} else {
				ic_got_reply = 1;
				ic_netdev = nd;
				break;
			}
		}
	}

	return nodev;
}

int do_bootp(void)
{
	int retries;
	unsigned int timeout;
	unsigned int start_csecs;

	bootp_print_config();

	printf("Sending BOOTP requests...");

	start_csecs = centisecs;
	retries = CONF_RETRIES;
	timeout = CONF_TIMEOUT_BASE;

	while (1) {
		unsigned int targ;

		if (bootp_send((centisecs - start_csecs)/100) < 0) {
			printf(" BOOTP failed\n");
			break;
		}

		printf(".");
		targ = centisecs + timeout;

		while (centisecs < targ && !ic_got_reply)
			bootp_recv();

		if (ic_got_reply) {
			printf(" OK\n");
			break;
		}

		if (retries && !--retries) {
			printf(" timed out\n");
			break;
		}

		timeout = timeout * CONF_TIMEOUT_MULT;
		if (timeout > CONF_TIMEOUT_MAX)
			timeout = CONF_TIMEOUT_MAX;
	}

	if (!ic_got_reply)
		return -1;

	printf("Got BOOTP answer from %s (%s) on %s, ",
		ic_bootp.serv_name,
		in_ntoa(ic_bootp.server_ip),
		ic_netdev->name);
	printf("my address is %s\n",
		in_ntoa(ic_bootp.your_ip));

	ic_netdev->ip_addr = ic_bootp.your_ip;

	return 0;
}
