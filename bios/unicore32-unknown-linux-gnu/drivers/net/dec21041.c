#include <bios/netdev.h>
#include <bios/pci.h>
#include <bios/if_ether.h>

struct netdev net_dev_dec_21041;
extern const struct trans_ops eth_trans_ops;

#define virt_to_bus(x)	((int)(x) + 0x10000000)

#define inw(x)		pci_io_read_word(nd->io_base + (x))
#define outw(w,x)	pci_io_write_word((w), nd->io_base + (x))
#define inl(x)		pci_io_read_long(nd->io_base + (x))
#define outl(w,x)	pci_io_write_long((w), nd->io_base + (x))

static char recvbuffer[1600];
static unsigned int recvdesc[4];
static unsigned int transdesc[4];

static int nd_21041_probe(void)
{
	struct netdev *nd = &net_dev_dec_21041;
	unsigned long pci_config_base;

	pci_config_base = pci_lookup_vendor_device(0x1011, 0x0014);

	if (pci_config_base) {
		nd->io_base = pci_read_config_long(pci_config_base, PCI_BASE0) & ~3;

		nd->hw_addr[0] = 0x45; /* E */
		nd->hw_addr[1] = 0x42; /* B */
		nd->hw_addr[2] = 0x53; /* S */
		nd->hw_addr[3] = 0x41; /* A */
		nd->hw_addr[4] = 0x44; /* D */
		nd->hw_addr[5] = 0x47; /* G */
		/* read h/w address */
		nd->hw_addr_len = 6;
	}

	return nd->io_base ? 0 : -1;
}

static int nd_21041_setmedia(struct netdev *nd, media_t media)
{
	return 0;
}

static int nd_21041_checkmedia(struct netdev *nd)
{
	return 0;
}

static int nd_21041_open(struct netdev *nd)
{
	unsigned int setupframe[3*16];
	int i;

	/* CSR0 - mus mode reg
	 *  no transmit poll, 16 lword burst, 8 word cache align, little endian
	 */
	outl(0x5006, 0);

	/* CSR7 - interrupt mask reg
	 *   no interrupts thanks
	 */
	outl(0, 0x38);

	/* Disable SIA
	 */
	outl(0, 0x68);

	/* From table 3-64 on p.94 of the 21041 hardware ref manual
	 * Use ef09,f7fd,0006 for 10base2 (Auto Negotiation) - seems to work for b2
	 * Use ef01,ffff,0008 for 10baseT (Auto Negotiation) - seems to work for bT
	 * Use ef09,0705,0006 for 10base2 (Auto sensing and negotiation disabled) - seems to work for b2
	 * Hang on - if its auto neg why the difference?
	 * We have to see if there is a link beat and if missing try the other
	 */

	/* CSR14 - SIA transmit/receive
	 */
	outl(0xf7fd, 0x70);

	/* CSR15 - SIA general
	 */
	outl(0x0006, 0x78);

	/* CSR13 connectivity reg
	 *  (must do last)
	 */
	outl(0xef09, 0x68);

	for (i = 0; i < 3*8; i += 3) {
		setupframe[i] = nd->hw_addr[0] | nd->hw_addr[1] << 8;
		setupframe[i + 1] = nd->hw_addr[2] | nd->hw_addr[3] << 8;
		setupframe[i + 2] = nd->hw_addr[4] | nd->hw_addr[5] << 8;
	}

	transdesc[0] = 0x80000000;	/* TDES0 - 21041 owns the descriptor */
	transdesc[1] = 0x0b0000c0;	/* TDES1 - End of ring, chained 2nd desc, setup, size of frame */
	transdesc[2] = virt_to_bus(setupframe); /* TDES2 - buffer 1 address */
	transdesc[3] = virt_to_bus(transdesc); /* TDES3 - next descriptor is same descriptor */

	outl(virt_to_bus(transdesc), 0x20);

	/* CSR6 - operating mode
	 *  kick the transmitter to process the setup frame
	 *  Enable special capture mode, bit17 enable capture mode,
	 *  large transmit threshold, enable transmit
	 */
	outl(0x8002e000, 0x30); 

	while (transdesc[0] & 0x80000000);

	/* RDES1 - end of ting, second address chained, sizeof buffer */
	recvdesc[1] = 0x03000000 | sizeof(recvbuffer);
	/* RDES2 - buffer address */
	recvdesc[2] = virt_to_bus(recvbuffer);
	/* RDES3 - next descriptor is same descriptor */
	recvdesc[3] = virt_to_bus(recvdesc);
	/* First/last descriptor, let 21041 own it */
	recvdesc[0] = 0x80000300;

	/* CSR3 - receive list base address reg
	 */
	outl(virt_to_bus(recvdesc), 0x18);

	/* Stop transmitter and start receiver
	 *   - As above, but no transmit enable, start receiver
	 */
	outl(0x8002c002, 0x30);

	return 0;
}

static int nd_21041_close(struct netdev *nd)
{
	return 0;
}

static int nd_21041_status(struct netdev *nd)
{
	return 0;
}

static int nd_21041_send(struct netdev *nd, void *buffer, int size)
{
	transdesc[0] = 0x80000000;
	transdesc[1] = size | 0x63000000;
	transdesc[2] = virt_to_bus(buffer);
	transdesc[3] = virt_to_bus(transdesc);

	/* CSR4 - transmit list base address
	 */
	outl(virt_to_bus(transdesc), 0x20);

	/* CSR6 - operating mode
	 *  setup operating mode and kick transmitter
	 *  - enable special capture, bit17 enable capture mode,
	 *    large transmit threshold, enable transmit
	 */
	outl(0x8002e002, 0x30);

	/* Spin waiting for the end of the transmission
	 */
	while (transdesc[0] & 0x80000000);

	/* CSR6 - operating mode
	 *  stop the transmitter and start receiver
	 */
	outl(0x8002c002, 0x30);

	return 0;
}

static int nd_21041_recv(struct netdev *nd, void *buf)
{
	int len = 0;

	switch (recvdesc[0] & 0x80008000) {
	case 0:
		len = (recvdesc[0] >> 16) & 0x7fff;
		memcpy(buf, recvbuffer, len);

	case 0x8000:	/* error */
		/* RDES1 - end of ting, second address chained, sizeof buffer */
		recvdesc[1] = 0x03000000 | sizeof(recvbuffer);
		/* RDES2 - buffer address */
		recvdesc[2] = virt_to_bus(recvbuffer);
		/* RDES3 - next descriptor is same descriptor */
		recvdesc[3] = virt_to_bus(recvdesc);
		/* First/last descriptor, let 21041 own it */
		recvdesc[0] = 0x80000300;
	
	default:
		break;
	}

	/* CSR6 - operating mode
	 *  make sure that the receiver is running
	 */
	outl(0x8002c002, 0x30);

	return len;
}

static const struct netdev_ops nd_21041_ops = {
	nd_21041_probe,
	nd_21041_open,
	nd_21041_close,
	nd_21041_status,
	nd_21041_setmedia,
	nd_21041_checkmedia,
	nd_21041_send,
	nd_21041_recv,
	"DEC 21041",
};

struct netdev net_dev_dec_21041 = {
	&nd_21041_ops,
	&eth_trans_ops,
	"21041",
};
