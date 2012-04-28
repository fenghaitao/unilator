#include <bios/netdev.h>
#include <bios/boot.h>
#include <bios/bootp.h>
#include <bios/ip.h>
#include <bios/udp.h>
#include <bios/string.h>
#include <bios/time.h>
#include <bios/stdio.h>

#define CONF_TIMEOUT_MINOR_BASE	10
#define CONF_TIMEOUT_MAJOR_BASE	500
#define CONF_TIMEOUT_MULT	5/4
#define CONF_TIMEOUT_MAX	1500
#define CONF_RETRIES		10

#define TFTP_RRQ		1
#define TFTP_DATA		3
#define TFTP_ACK		4
#define TFTP_ERROR		5

#define TFTP_STATE_RRQ		0
#define TFTP_STATE_ACK		1
#define TFTP_STATE_COMPLETE	2

struct tftp_rrq {
	u16	cmd;
	u8	filename[100];
};

struct tftp_ack {
	u16	cmd;
	u16	block;
};

struct tftp_data {
	u16	cmd;
	u16	block;
	u8	data[512];
};

extern struct bootp_pkt	 ic_bootp;
extern struct netdev	*ic_netdev;
static struct sin	 tftp_me, tftp_serv;

static int tftp_send_rrq(void)
{
	struct tftp_rrq rrq;
	struct buflist bl;

	tftp_me.sin_addr = ic_netdev->ip_addr;
	tftp_me.sin_port = htons(0x8000);

	tftp_serv.sin_addr = ic_bootp.server_ip;
	tftp_serv.sin_port = htons(0x45);

	memzero(&rrq, sizeof(rrq));
	rrq.cmd = htons(TFTP_RRQ);
	sprintf(rrq.filename, "%s%c%s", ic_bootp.boot_file, 0, "octet");

	bl.data = &rrq;
	bl.size = 102;
	bl.next = NULL;

	return udp_send(ic_netdev, &tftp_me, &tftp_serv, &bl);
}

static int tftp_send_ack(int block)
{
	struct tftp_ack ack;
	struct buflist bl;

	ack.cmd   = htons(TFTP_ACK);
	ack.block = htons(block);

	bl.data = &ack;
	bl.size = 102;
	bl.next = NULL;

	return udp_send(ic_netdev, &tftp_me, &tftp_serv, &bl);
}

static int tftp_recv_data(int block, void *buffer)
{
	struct tftp_data data;
	int bytes, recvd_block;

	if (block == 1)
		tftp_serv.sin_port = 0;

	bytes = udp_recv(ic_netdev, &tftp_serv, &tftp_me, &data, sizeof(data));

	if (!bytes)
		return 0;

	if (data.cmd != htons(TFTP_DATA))
		return 0;

	recvd_block = htons(data.block);

	if (recvd_block <= block)
		tftp_send_ack(recvd_block);

	if (recvd_block != block)
		return 0;

	memcpy(buffer, data.data, bytes - 4);

	return bytes - 4;
}

#define TWIDDLE_IDX(x)	  (((x) >> 16) & 3)
#define TWIDDLE_UPDATE(x) (((x) & 0xffff) == 0)

int do_tftp(void)
{
	static char twiddle[] = { '|', '/', '-', '\\' };
	int timeout, targ, retries, error, block = 1, bytes = 0;
	int total_bytes = 0;
	unsigned char *data = (unsigned char *)load_addr;

	printf("TFTPing %s... ", ic_bootp.boot_file);

	timeout = CONF_TIMEOUT_MAJOR_BASE;
	retries = CONF_RETRIES;

	do {
		if (block == 1)
			error = tftp_send_rrq();
		else
			error = tftp_send_ack(block);

		if (error)
			break;

		targ = centisecs + timeout;

		while (centisecs < targ) {
			bytes = tftp_recv_data(block, data + total_bytes);
			if (bytes) {
				retries = CONF_RETRIES;
				timeout = CONF_TIMEOUT_MINOR_BASE;
				targ = centisecs + timeout;
				if (TWIDDLE_UPDATE(total_bytes))
					printf("%c\010", twiddle[TWIDDLE_IDX(total_bytes)]);
				total_bytes += bytes;
				if (bytes == 512)
					block ++;
				else
					break;
			}
		}

		if (timeout == CONF_TIMEOUT_MINOR_BASE)
			timeout = CONF_TIMEOUT_MAJOR_BASE;

		if (bytes)
			break;

		if (!--retries) {
			printf("\010 timed out\n");
			error = 1;
			break;
		}

		timeout = timeout * CONF_TIMEOUT_MULT;
		if (timeout > CONF_TIMEOUT_MAX)
			timeout = CONF_TIMEOUT_MAX;
	} while (1);

	if (error) {
		printf("\010 Error\n");
		return 1;
	}

	printf("\010 Ok - %dKB\n", total_bytes / 1024);

	return 0;
}
