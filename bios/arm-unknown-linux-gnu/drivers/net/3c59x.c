#include <bios/netdev.h>
#include <bios/pci.h>
#include <bios/if_ether.h>

struct netdev net_dev_3com_3c59x;
extern const struct trans_ops eth_trans_ops;

#define inw(x)		pci_io_read_word(nd->io_base + (x))
#define outw(w,x)	pci_io_write_word((w), nd->io_base + (x))
#define inl(x)		pci_io_read_long(nd->io_base + (x))
#define outl(w,x)	pci_io_write_long((w), nd->io_base + (x))

#define Command				14
#define Command_SelectRegisterWindow(x)	(0x0800 | (x))
#define Command_EnableDcConverter	(0x1000)
#define Command_RxDisable		(0x1800)
#define Command_RxEnable		(0x2000)
#define Command_RxReset(mask)		(0x2800|(mask))
#define Command_TxDone			(0x3800)
#define Command_RxDiscard		(0x4000)
#define Command_TxEnable		(0x4800)
#define Command_TxDisable		(0x5000)
#define Command_TxReset(mask)		(0x5800|(mask))
#define Command_RequestInterrupt	(0x6000)
#define Command_AcknowledgeInterrupt(m)	(0x6800|(m))
#define Command_SetInterruptEnable(m)	(0x7000|(m))
#define Command_SetIndicationEnable(m)	(0x7800|(m))
#define Command_SetRxFilter(m)		(0x8000|(m))
#define Command_SetRxEarlyThresh(t)	(0x8800|(t))
#define Command_SetTxAvailableThresh(t)	(0x9000|(t))
#define Command_SetTxStartThresh(t)	(0x9800|(t))
#define Command_StartDma(mode)		(0xa000|(mode))
#define Command_StatisticsEnable	(0xa800)
#define Command_StatisticsDisable	(0xb000)
#define Command_DisableDcConverter	(0xb800)

#define IntStatus			14
#define IntStatus_interruptLatch	(1 << 0)
#define IntStatus_hostError		(1 << 1)
#define IntStatus_txComplete		(1 << 2)
#define IntStatus_txAvailable		(1 << 3)
#define IntStatus_rxComplete		(1 << 4)
#define IntStatus_rxEarly		(1 << 5)
#define IntStatus_intRequested		(1 << 6)
#define IntStatus_updateStats		(1 << 7)
#define IntStatus_transferInt		(1 << 8)
#define IntStatus_busMasterInProgress	(1 << 11)
#define IntStatus_commandInProgress	(1 << 12)
#define IntStatus_windowNumber		(7 << 13)

/* Bank 1 */
#define RxData				0

#define RxStatus			8
#define RxStatus_rxBytes		(0x1fff)
#define RxStatus_rxError		(1 << 14)
#define RxStatus_rxIncomplete		(1 << 15)

/* Bank 3 */
#define InternalConfig			0
#define InternalConfig_autoSelect	(1 << 24)
#define InternalConfig_xcvrSelect(x)	((x) << 20)

#define MacControl			6
#define MacControl_fullDuplexEnable	(1 << 5)

/* Bank 4 */
#define MediaStatus			10
#define MediaStatus_dataRate100		(1 << 1)	/* ro */
#define MediaStatus_crcStripDisable	(1 << 2)	/* rw */
#define MediaStatus_enableSqeStats	(1 << 3)	/* ro */
#define MediaStatus_collisionDetect	(1 << 4)	/* ro */
#define MediaStatus_carrierSense	(1 << 5)	/* ro */
#define MediaStatus_jabberGuardEnable	(1 << 6)	/* rw */
#define MediaStatus_linkBeatEnable	(1 << 7)	/* rw */
#define MediaStatus_jabberDetect	(1 << 9)	/* ro */
#define MediaStatus_polarityReversed	(1 << 10)	/* ro */
#define MediaStatus_linkBeatDetect	(1 << 11)	/* ro */
#define MediaStatus_txInProg		(1 << 12)	/* ro */
#define MediaStatus_dcConverterEnabled	(1 << 14)	/* ro */
#define MediaStatus_auiDisable		(1 << 15)	/* ro */

static int eeprom_read(struct netdev *nd, int offset)
{
	outw(Command_SelectRegisterWindow(0), Command);
	while ((inw(10) & 1 << 15) != 0);

	outw(0x0080 | (offset & 0x3f), 10);
	while ((inw(10) & 1 << 15) != 0);

	return inw(12);
}

static inline void wait_cmd(struct netdev *nd)
{
	while ((inw(IntStatus) & IntStatus_commandInProgress) != 0);
}

static void send_and_wait_cmd(struct netdev *nd, int cmd)
{
	outw(cmd, Command);
	wait_cmd(nd);
}

static int nd_3c59x_probe(void)
{
	struct netdev *nd = &net_dev_3com_3c59x;
	unsigned long pci_config_base;
	int i;

	pci_config_base = pci_lookup_vendor_device(0x10b7, 0x5900);
	if (!pci_config_base)
		pci_config_base = pci_lookup_vendor_device(0x10b7, 0x5950);
	if (!pci_config_base)
		pci_config_base = pci_lookup_vendor_device(0x10b7, 0x5951);
	if (!pci_config_base)
		pci_config_base = pci_lookup_vendor_device(0x10b7, 0x5952);

	if (pci_config_base) {
		nd->io_base = pci_read_config_long(pci_config_base, PCI_BASE0) & ~3;

		for (i = 0; i < 3; i++) {
			int addr;

			addr = eeprom_read(nd, i + 10);

			nd->hw_addr[i * 2] = addr >> 8;
			nd->hw_addr[i * 2 + 1] = addr;
		}
		eth_setup(nd);
	}

	return net_dev_3com_3c59x.io_base ? 0 : -1;
}

static int nd_3c59x_setmedia(struct netdev *nd, media_t media)
{
	unsigned int v;

	outw(Command_SelectRegisterWindow(3), Command);

	// Set to 10bT
	v = inl(InternalConfig);
	v &= ~(InternalConfig_autoSelect | InternalConfig_xcvrSelect(7));

	switch (media) {
	case media_utp:
		outw(0, MacControl);
//		outw(MacControl_fullDuplexEnable, MacControl);
		break;

	case media_coax:
		outw(0, MacControl);
		v |= InternalConfig_xcvrSelect(3);
		break;

	case media_aui:
		outw(0, MacControl);
		v |= InternalConfig_xcvrSelect(1);
		break;

	default:
		return -1;
	}

	outl(v, InternalConfig);

	send_and_wait_cmd(nd, Command_TxReset(0));	// Tx Reset
	send_and_wait_cmd(nd, Command_RxReset(0));	// Rx Reset

	outw(Command_SelectRegisterWindow(4), Command);
	v = inw(MediaStatus);

	if (media == media_utp)
		v |= MediaStatus_jabberGuardEnable | MediaStatus_linkBeatEnable;
	else
		v &= ~(MediaStatus_jabberGuardEnable | MediaStatus_linkBeatEnable);

	outw(v, MediaStatus);
	return 0;
}

static int nd_3c59x_checkmedia(struct netdev *nd)
{
	return 0;
}

static int nd_3c59x_open(struct netdev *nd)
{
	nd_3c59x_setmedia(nd, media_utp);

	outw(Command_SetRxFilter(5), Command);		// SetRxFilter
	outw(Command_SelectRegisterWindow(2), Command);	// Reg window 2
	outw(nd->hw_addr[0] | nd->hw_addr[1] << 8, 0);
	outw(nd->hw_addr[2] | nd->hw_addr[3] << 8, 2);
	outw(nd->hw_addr[4] | nd->hw_addr[5] << 8, 4);
	outw(0, 6);					// StationMask
	outw(0, 8);
	outw(0, 10);

	outw(Command_TxEnable, Command);		// TxEnable
	outw(Command_RxEnable, Command);		// RxEnable

	outw(Command_SetIndicationEnable(0x7ff), Command);// SetIndicationEnable
	outw(Command_StatisticsEnable, Command);	// StatisticsEnable

	return 0;
}

static int nd_3c59x_close(struct netdev *nd)
{
	return 0;
}

static int nd_3c59x_status(struct netdev *nd)
{
	return 0;
}

static int nd_3c59x_send(struct netdev *nd, void *buffer, int size)
{
	unsigned long *frame = (unsigned long *)buffer;

	outw(Command_SelectRegisterWindow(1), Command);

	while (inw(12) < size);

	outl(size, 0);

	while (size > 0) {
		outl(*frame++, 0);
		size -= 4;
	}

	return 0;
}

static int nd_3c59x_recv(struct netdev *nd, void *buf)
{
	unsigned long *frame = (unsigned long *)buf;
	int length = 0;

	if (inw(IntStatus) & IntStatus_rxComplete) {
		int rxstat;

		outw(Command_SelectRegisterWindow(1), Command);

		rxstat = inw(RxStatus);

		if (!(rxstat & RxStatus_rxError)) {
			int i = length = rxstat & RxStatus_rxBytes;

			while (i > 0) {
				i -= 4;
				*frame++ = inl(RxData);
			}
		}
		outw(Command_RxDiscard, Command);
	}
	return length;
}

static const struct netdev_ops nd_3c59x_ops = {
	nd_3c59x_probe,
	nd_3c59x_open,
	nd_3c59x_close,
	nd_3c59x_status,
	nd_3c59x_setmedia,
	nd_3c59x_checkmedia,
	nd_3c59x_send,
	nd_3c59x_recv,
	"3com 3c59x",
};

struct netdev net_dev_3com_3c59x = {
	&nd_3c59x_ops,
	&eth_trans_ops,
	"3c59x",
};
