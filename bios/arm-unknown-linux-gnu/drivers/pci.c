#include <bios/dec21285.h>
#include <bios/pci.h>
#include <bios/machine.h>
#include <bios/stdio.h>

static unsigned long pci_io_free;
static unsigned long pci_mem_free;

#ifndef DEBUG
#define debug_printf(x...)
#endif

#define FUNC(x) ((x) & 7)
#define SLOT(x) ((x) >> 3)
#define MAX_DEV (20 << 3)

#if 0
static char irqmap = { 9, 8, 18, 11 };

static int ebsa_irqval(struct pci_dev *dev)
{
	unsigned char pin;

	pcibios_read_config_byte(dev->bus->number,
	               dev->devfn,
	               PCI_INTERRUPT_PIN,
	               &pin);

	return irqmap_ebsa[(PCI_SLOT(dev->devfn) + pin) & 3];
}
#endif

static inline unsigned long pci_alloc(unsigned long *resource, unsigned long size)
{
	unsigned long base;

	*resource += size - 1;
	*resource &= ~(size - 1);

	base = *resource;

	*resource += size;

	return base;
}

static unsigned long pci_alloc_io(unsigned long size)
{
	return pci_alloc(&pci_io_free, size);	
}

static unsigned long pci_alloc_mem(unsigned long size)
{
	return pci_alloc(&pci_mem_free, size);
}

static void pci_init_hw(void)
{
	unsigned int ctrl = csr_read_long(CSR_CTRL);

	if (ctrl & (1 << 31)) {
		csr_write_long(0, CSR_PCIEXTN);

		/*
		 * Set up the mask registers
		 */
		csr_write_long(12, CSR_OIMR);
		csr_write_long(0, CSR_DPMR);
		csr_write_long(0, CSR_DSMR);

		/*
		 * Disable PCI reset
		 */
		ctrl |= (1 << 9);
		csr_write_long(ctrl, CSR_CTRL);

		/*
		 * Disable our PCI interface
		 */
		csr_write_word(0, CSR_PCI_CMD);

		/*
		 * Set up the PCI -> Host mappings
		 */
		csr_write_long(0, CSR_CSRBASEADDRMASK);
		csr_write_long(0x40000000, CSR_CSRMEMBASE);
		csr_write_long(0x0000f000, CSR_CSRIOBASE);

		/*
		 * SDRAM is mapped at PCI space address 0x10000000
		 */
		csr_write_long(0x01fc0000, CSR_SDRAMBASEADDRMASK);
		csr_write_long(0x10000000, CSR_SDRAMBASE);
		csr_write_long(0, CSR_SDRAMBASEOFF);
		csr_write_word((0x17 | (1 << 9)), CSR_PCI_CMD);

		/*
		 * Set Init Complete
		 */
		csr_write_long(ctrl | CSR_CTRL_INITCOMPLETE, CSR_CTRL);
	}
}

static void pci_setup_cards_0(unsigned long pci_base, unsigned int *pci_command)
{
	unsigned long base, size;
	unsigned int i, class;

	for (i = 0; i < 6*4; i += 4) {
		pci_write_config_long(pci_base, PCI_BASE0 + i, 0xffffffff);

		base = pci_read_config_long(pci_base, PCI_BASE0 + i);

		if (base == 0)
			continue;

		if (base & 1) {	/* IO space */
			size = -(base & ~3);

			size &= 0xffff;

			base = pci_alloc_io(size) | 1;

			*pci_command |= PCI_COMMAND_IO_SPACE;
		} else {	/* MEM space */
			size = -(base & ~15);

			base = pci_alloc_mem(size);

			*pci_command |= PCI_COMMAND_MEM_SPACE;
		}
		pci_write_config_long(pci_base, PCI_BASE0 + i, base);
		if (pci_read_config_long(pci_base, PCI_BASE0 + i) != base)
			debug_printf("Unable to set base\n");
	}
//debug_printf("MinGnt = %d, MaxGnt = %d\n", pci_read_config_byte(pci_base, PCI_MINGNT),
//	pci_read_config_byte(pci_base, PCI_MAXLAT));

	pci_write_config_byte(pci_base, PCI_LATENCY_TIMER, 32);

	pci_write_config_long(pci_base, PCI_BIOSROMCONTROL, -2);
	base = pci_read_config_long(pci_base, PCI_BIOSROMCONTROL);

	if (base) {
		size = -(base & ~1);

		base = pci_alloc_mem(size) | 1;

		pci_write_config_long(pci_base, PCI_BIOSROMCONTROL, base);

		*pci_command |= PCI_COMMAND_MEM_SPACE;
	}

	class = pci_read_config_long(pci_base, PCI_CLASS_CODE) >> 24;
	switch (class) {
	case PCI_CLASS_NOT_DEFINED:
	case PCI_BASE_CLASS_DISPLAY:
	case PCI_BASE_CLASS_STORAGE:
		*pci_command |= PCI_COMMAND_IO_SPACE;
		break;
	}
}

struct pcidev {
	unsigned int dev;
	unsigned int vendor;
	unsigned int device;
	unsigned int command;
	unsigned int class;
	unsigned int header;
	unsigned long pci_base;
};

struct pcidev pcidev;

static void pci_scan(void (*fn)(struct pcidev *))
{
	int dev, multi = 0;

	for (dev = 0; dev < MAX_DEV; dev++) {
		unsigned long pci_base;

		if (FUNC(dev) && !multi)
			continue;

		pci_base = pci_config_addr(SLOT(dev), FUNC(dev));

		pcidev.vendor = pci_read_config_word(pci_base, PCI_VENDOR_ID);
		pcidev.device = pci_read_config_word(pci_base, PCI_DEVICE_ID);

		if (pcidev.vendor == 0xffff ||
		    pcidev.vendor == 0x0000 ||
		    pcidev.device == 0xffff ||
		    pcidev.device == 0x0000)
			continue;

		pcidev.header = pci_read_config_byte(pci_base, PCI_HEADER_TYPE);
		pcidev.command = pci_read_config_word(pci_base, PCI_COMMAND);
		pcidev.class = pci_read_config_long(pci_base, PCI_CLASS_CODE) >> 16;
		pcidev.pci_base = pci_base;
		pcidev.dev = dev;

		multi = pcidev.header & 0x80;

		fn(&pcidev);
	}
}

static void pci_setup_card(struct pcidev *dev)
{
	dev->command &= ~(PCI_COMMAND_BUS_MASTER |
			  PCI_COMMAND_MEM_SPACE |
			  PCI_COMMAND_IO_SPACE);

	if ((dev->header & 0x7f) == 0)
		pci_setup_cards_0(dev->pci_base, &dev->command);

	pci_write_config_word(dev->pci_base, PCI_COMMAND, dev->command);
}

static void pci_setup_cards(void)
{
	pci_scan(pci_setup_card);
}

static void pci_print_card(struct pcidev *dev)
{
	unsigned int addr[4];
	int i;

	printf("%2d.%d", SLOT(dev->dev), FUNC(dev->dev));
	printf("  %04X   %04X  %04X  %c%c%c",
	       dev->vendor, dev->device, dev->class,
	       dev->command & PCI_COMMAND_BUS_MASTER ? '*' : ' ',
	       dev->command & PCI_COMMAND_MEM_SPACE  ? '*' : ' ',
	       dev->command & PCI_COMMAND_IO_SPACE   ? '*' : ' ');

	addr[0] = pci_read_config_long(dev->pci_base, PCI_BASE0);
	addr[1] = pci_read_config_long(dev->pci_base, PCI_BASE1);
	addr[2] = pci_read_config_long(dev->pci_base, PCI_BASE2);
	addr[3] = pci_read_config_long(dev->pci_base, PCI_BIOSROMCONTROL);

	for (i = 0; i < 4; i++)
		if (addr[i])
			printf(" %08X", addr[i]);
		else
			printf("         ");
	printf("\n");
}

void pci_print_config(void)
{
	printf("---------------------- PCI Configuration -----------------------\n");
	printf("Slot Vendor Device Class BMI Address0 Address1 Address2 Bios\n");

	pci_scan(pci_print_card);

	printf("----------------------------------------------------------------\n");
}

unsigned long pci_lookupclass(unsigned short class)
{
	unsigned long pci_base = 0;
	int dev, multi = 0;

	for (dev = 0; dev < MAX_DEV; dev++) {
		unsigned int vendor_id, header, card_class, cmd;

		if (FUNC(dev) && !multi)
			continue;

		pci_base = pci_config_addr(SLOT(dev), FUNC(dev));

		vendor_id = pci_read_config_word(pci_base, PCI_VENDOR_ID);

		if (vendor_id == 0xffff || vendor_id == 0)
			continue;

		header = pci_read_config_byte(pci_base, PCI_HEADER_TYPE);
		cmd = pci_read_config_word(pci_base, PCI_COMMAND);
		multi = header & 0x80;

		card_class = pci_read_config_long(pci_base, PCI_CLASS_CODE) >> 16;

		if (card_class == class && cmd & (PCI_COMMAND_MEM_SPACE|PCI_COMMAND_IO_SPACE))
			break;
	}

	return dev < MAX_DEV ? pci_base : 0;
}

unsigned long pci_lookup_vendor_device(unsigned short vendor_id, unsigned short device_id)
{
	unsigned long pci_base = 0;
	int dev, multi = 0;

	for (dev = 0; dev < MAX_DEV; dev++) {
		unsigned int cvendor_id, cdevice_id, header;

		if (FUNC(dev) && !multi)
			continue;

		pci_base = pci_config_addr(SLOT(dev), FUNC(dev));

		cvendor_id = pci_read_config_word(pci_base, PCI_VENDOR_ID);
		cdevice_id = pci_read_config_word(pci_base, PCI_DEVICE_ID);

		if (cvendor_id == 0xffff || cvendor_id == 0)
			continue;

		header = pci_read_config_byte(pci_base, PCI_HEADER_TYPE);
		multi = header & 0x80;

		if (vendor_id == cvendor_id && device_id == cdevice_id)
			break;
	}

	return dev < MAX_DEV ? pci_base : 0;
}

void pci_init(void)
{
	pci_io_free  = 0x8000;
	pci_mem_free = PCIMEM_BASE;

	pci_init_hw();
	pci_setup_cards();
}
