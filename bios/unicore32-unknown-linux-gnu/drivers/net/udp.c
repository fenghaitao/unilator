#include <bios/netdev.h>
#include <bios/checksum.h>
#include <bios/udp.h>

#ifndef DEBUG
#define debug_printf(x...)
#endif

static unsigned short
udp_check(struct udphdr *hdr, struct sin *from, struct sin *to, struct buflist *data, int size)
{
	struct buflist *blp;
	int check;

	check = checksum(&from->sin_addr, sizeof(from->sin_addr), 0x11 + size);
	check = checksum(&to->sin_addr, sizeof(to->sin_addr), check);

	for (blp = data; blp; blp = blp->next)
		check = checksum(blp->data, blp->size, check);

	check = ~check;

	if (check == 0)
		check = -1;

	return htons(check);
}

int udp_send(struct netdev *nd, struct sin *from, struct sin *to, struct buflist *data)
{
	struct buflist bl, *blp;
	struct udphdr udp;
	int size = 0;

	bl.data = &udp;
	bl.size = sizeof(udp);
	bl.next = data;

	for (blp = &bl; blp; blp = blp->next)
		size += blp->size;

	udp.udp_source = from->sin_port;
	udp.udp_dest   = to->sin_port;
	udp.udp_length = htons(size);
	udp.udp_check  = 0;
	udp.udp_check  = udp_check(&udp, from, to, &bl, size);

	return ip_send(nd, 0x11, from->sin_addr, to->sin_addr, &bl);
}

int udp_recv(struct netdev *nd, struct sin *from, struct sin *to,
	     void *buffer, int size)
{
	u8 udp_buffer[1564];
	struct udphdr *udp = (struct udphdr *)udp_buffer;
	int bytes;

	bytes = ip_recv(nd, 0x11, from->sin_addr, to->sin_addr, udp_buffer);

	if (bytes) {
		if (bytes < sizeof(struct udphdr)) {
			debug_printf("invalid length %d\n", bytes);
			return 0;
		}

		if (bytes < htons(udp->udp_length)) {
			debug_printf("wrong length (%d < %d)", bytes, htons(udp->udp_length));
			return 0;
		}

		if (to->sin_port != 0) {
			if (to->sin_port != udp->udp_dest) {
				debug_printf("wrong dest port\n");
				return 0;
			}
		} else
			to->sin_port = udp->udp_dest;

		if (from->sin_port != 0) {
			if (from->sin_port != udp->udp_source) {
				debug_printf("wrong source port\n");
				return 0;
			}
		} else
			from->sin_port = udp->udp_source;

		bytes -= sizeof(*udp);
		memcpy(buffer, udp_buffer + sizeof(*udp), bytes);
	}

	return bytes;
}

