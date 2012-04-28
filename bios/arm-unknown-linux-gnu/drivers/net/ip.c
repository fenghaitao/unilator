#include <bios/netdev.h>
#include <bios/checksum.h>
#include <bios/buflist.h>
#include <bios/ip.h>
#include <bios/if_ether.h>
#include <bios/stdio.h>

#ifndef DEBUG
#define debug_printf(x...)
#endif

static u32 ip_pkt_no;

char *in_ntoa(u32 addr)
{
	static char buffer[16];

	sprintf(buffer, "%ld.%ld.%ld.%ld",
		addr & 255,
		(addr >> 8) & 255,
		(addr >> 16) & 255,
		(addr >> 24) & 255);

	return buffer;
}

static unsigned short ip_check(struct iphdr *ip)
{
	unsigned int check, length = ip->ip_ihl * 4;

	check = ~checksum(ip, length, 0);

	if (check == 0)
		check = -1;

	return htons(check);
}

int ip_send(struct netdev *nd, int protocol, u32 from, u32 to, struct buflist *data)
{
	u8 hwto[6], *dest;
	struct buflist bl, *blp;
	struct iphdr ip;
	int size = 0;

	for (blp = data; blp; blp = blp->next)
		size += blp->size;

	ip.ip_ver    = 4;
	ip.ip_ihl    = 5;
	ip.ip_tos    = 0;
	ip.ip_len    = htons(sizeof(ip) + size);
	ip.ip_id     = htons(ip_pkt_no);
	ip.ip_frag   = 0;
	ip.ip_ttl    = 0x40;
	ip.ip_proto  = protocol;
	ip.ip_check  = 0;
	ip.ip_source = from;
	ip.ip_dest   = to;
	ip.ip_check  = ip_check(&ip);

	ip_pkt_no += 1;

	bl.data = &ip;
	bl.size = sizeof(ip);
	bl.next = data;

	if (to == INADDR_ANY)
		dest = nd->hw_broadcast;
	else if (!arp_lookup(nd, to, hwto))
		dest = hwto;
	else
		return -1;
	
	blp = nd->trans->header(nd, dest, ETH_P_IP, &bl);

	return blp ? nd->trans->send(nd, blp) : -1;
}

int ip_recv(struct netdev *nd, int protocol, u32 from, u32 to, u8 *buffer)
{
	u8 ip_buffer[1584];
	struct iphdr *ip = (struct iphdr *)ip_buffer;
	int bytes, hdrlen;

	bytes = nd->trans->recv(nd, ETH_P_IP, ip_buffer);

	if (bytes == 0)
		return 0;

	if (ip->ip_ver != 4 || ip->ip_ihl < 5) {
		debug_printf("wrong IP version/header length\n");
		return 0;
	}

	hdrlen = ip->ip_ihl * 4;

	if (checksum(ip, hdrlen, 0) != 0xffff) {
		debug_printf("bad IP checksum\n");
		return 0;
	}

	if (ip->ip_proto != protocol) {
		debug_printf("wrong protocol\n");
		return 0;
	}

	if (from != INADDR_ANY && from != ip->ip_source) {
		debug_printf("wrong source address\n");
		return 0;
	}

	if (to != INADDR_ANY && to != ip->ip_dest) {
		debug_printf("wrong destination address\n");
		return 0;
	}

	bytes -= hdrlen;

	memcpy(buffer, ip_buffer + hdrlen, bytes);

	return bytes;
}
