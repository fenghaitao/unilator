#include <bios/buflist.h>
#include <bios/netdev.h>
#include <bios/if_ether.h>
#include <bios/string.h>

static u8 ether_buffer[1600];

void eth_setup(struct netdev *nd)
{
	int i;

	nd->hw_addr_len = 6;

	for (i = 0; i < nd->hw_addr_len; i++)
		nd->hw_broadcast[i] = 0xff;
}

static struct buflist *eth_header(struct netdev *nd, u8 *to, u16 proto, struct buflist *blp)
{
	static struct buflist bl;
	static struct ethhdr  eth;

	bl.data = &eth;
	bl.size = 14;
	bl.next = blp;

	memcpy(eth.eth_source, nd->hw_addr, nd->hw_addr_len);
	memcpy(eth.eth_dest, to, nd->hw_addr_len);
	eth.eth_proto = htons(proto);

	return &bl;
}

static int eth_send(struct netdev *nd, struct buflist *data)
{
	struct buflist *blp;
	int size = 0;

	for (blp = data; blp; blp = blp->next) {
		memcpy(ether_buffer + size, blp->data, blp->size);
		size += blp->size;
	}

	return nd->hard->send(nd, ether_buffer, size);
}

static int eth_recv(struct netdev *nd, u16 proto, u8 *buffer)
{
	struct ethhdr *eth = (struct ethhdr *)ether_buffer;
	int bytes;

	do {
		bytes = nd->hard->recv(nd, ether_buffer);

		if (bytes < 14)
			continue;

		if (!memeq(eth->eth_dest, nd->hw_addr, nd->hw_addr_len))
			continue;

		if (eth->eth_proto == htons(proto)) {
			bytes -= 14;
			memcpy(buffer, ether_buffer + 14, bytes);
			break;
		}
	} while (bytes);

	return bytes;
}

const struct trans_ops eth_trans_ops = {
	eth_header,
	eth_send,
	eth_recv
};
