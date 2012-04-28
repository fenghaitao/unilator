#include <bios/types.h>
#include <bios/netdev.h>
#include <bios/ip.h>
#include <bios/if_ether.h>
#include <bios/string.h>
#include <bios/time.h>

static u32	cached_ip;
static u8	cached_hw[8];

struct arphdr {
	u16	ar_hrd;		/* format of hardware address	*/
	u16	ar_pro;		/* format of protocol address	*/
	u8	ar_hln;		/* length of hardware address	*/
	u8	ar_pln;		/* length of protocol address	*/
	u16	ar_op;		/* ARP opcode (command)		*/
	u8	ar_sha[6];	/* sender h/w address		*/
	u8	ar_sip[4];	/* sender ip address		*/
	u8	ar_tha[6];	/* target h/w address		*/
	u8	ar_tip[4];	/* target ip address		*/
};

/*
 * Send an ARP request to resolve an IP address
 */
static int arp_send(struct netdev *nd, u32 ip_addr)
{
	struct arphdr ar;
	struct buflist bl, *blp;

	bl.data = &ar;
	bl.size = sizeof(ar);
	bl.next = NULL;

	memzero(&ar, sizeof(ar));

	ar.ar_hrd = htons(1);		/* ARPHRD_ETHER */
	ar.ar_pro = htons(ETH_P_IP);
	ar.ar_hln = nd->hw_addr_len;
	ar.ar_pln = sizeof(ip_addr);
	ar.ar_op  = htons(1);		/* ARPOP_REQUEST */
	memcpy(ar.ar_sha, nd->hw_addr, nd->hw_addr_len);
	memcpy(ar.ar_sip, &nd->ip_addr, sizeof(nd->ip_addr));
	memcpy(ar.ar_tip, &ip_addr, sizeof(ip_addr));

	blp = nd->trans->header(nd, nd->hw_broadcast, ETH_P_ARP, &bl);

	return blp ? nd->trans->send(nd, blp) : -1;
}

/*
 * Check ARP reply is valid for the specified IP address
 */
static int arp_reply_valid(struct netdev *nd, u32 ip_addr, struct arphdr *ar)
{
	int valid;

	valid = ar->ar_hrd == htons(1);			/* ARPHRD_ETHER */
	valid = valid && ar->ar_pro == htons(ETH_P_IP);
	valid = valid && ar->ar_hln == nd->hw_addr_len;
	valid = valid && ar->ar_pln == sizeof(ip_addr);
	valid = valid && ar->ar_op == htons(2);		/* ARPOP_REPLY */
	valid = valid && memeq(ar->ar_tha, nd->hw_addr, nd->hw_addr_len);
	valid = valid && memeq(ar->ar_tip, &nd->ip_addr, sizeof(nd->ip_addr));
	valid = valid && memeq(ar->ar_sip, &ip_addr, sizeof(ip_addr));

	return valid;
}

/*
 * Get an Ether address for an IP address
 */
int arp_lookup(struct netdev *nd, u32 addr, u8 *eth_addr)
{
	int retries = 15, timeout = 25;

	while(addr != cached_ip && retries) {
		char buffer[1024];
		struct arphdr *ar = (struct arphdr *)buffer;
		int bytes;
		int next_try;

		arp_send(nd, addr);

		next_try = centisecs + timeout;
		timeout = timeout * 2;
		if (timeout > 500)
			timeout = 500;

		do {
			bytes = nd->trans->recv(nd, ETH_P_ARP, buffer);

			if (bytes >= sizeof(*ar) && arp_reply_valid(nd, addr, ar))
				break;
			bytes = 0;
		} while (centisecs < next_try);

		retries -= 1;

		if (!bytes)
			continue;

		cached_ip = addr;
		memcpy(cached_hw, ar->ar_sha, ar->ar_hln);
	}

	if (addr == cached_ip) {
		memcpy(eth_addr, cached_hw, nd->hw_addr_len);
		return 0;
	}

	return 1;
}
