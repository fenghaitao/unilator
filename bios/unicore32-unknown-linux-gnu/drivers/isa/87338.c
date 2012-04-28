#include <bios/pci.h>
#include <bios/stdio.h>

#define inb(r)		pci_io_read_byte((r))
#define outb(v,r)	pci_io_write_byte((v),(r))

struct {
	char reg;
	char mask;
	char val;
} regs[] = {
	{ 0x00, 0xc0, 0x00 },	/* FER */
	{ 0x51, 0xf0, 0x05 },	/* CLK   - 24MHz clock			  */
	{ 0x01, 0x00, 0x10 },	/* FAR */
	{ 0x02, 0x22, 0x08 },	/* PTR */
	{ 0x03, 0x14, 0x21 },	/* FCR */
	{ 0x04, 0x90, 0x04 },	/* PCR 03*/
	{ 0x06, 0x89, 0x00 },	/* PMC */
	{ 0x07, 0x11, 0x00 },	/* TUP 04*/
	{ 0x09, 0x02, 0xc4 },	/* ASC */
	{ 0x0a, 0x00, 0x00 },	/* CS0LA */
	{ 0x0b, 0xc7, 0x00 },	/* CS0CF */
	{ 0x0c, 0x00, 0x00 },	/* CS1LA */
	{ 0x0d, 0xc7, 0x00 },	/* CS1CF */
	{ 0x10, 0x00, 0x00 },	/* CS0HA */
	{ 0x11, 0x00, 0x00 },	/* CS1HA */
	{ 0x12, 0xf7, 0x00 },	/* SCF0  - SCC2 enabled			  */
	{ 0x18, 0xc1, 0x0a },	/* SCF1  - PPort DMA3 (DRQ0)		  */
	{ 0x1b, 0x00, 0x79 },	/* PNP0  - PPort IRQ7, PnP mode, ECP IRQ7 */
	{ 0x1c, 0x00, 0x34 },	/* PNP1  - SCC1 IRQ4, SCC2 IRQ3		  */
	{ 0x40, 0x0c, 0x20 },	/* SCF2  - SCC2 normal power		  */
	{ 0x41, 0x80, 0x36 },	/* PNP2  - FDC DMA 2 (DRQ2), FDC IRQ 6	  */
	{ 0x42, 0x00, 0xde },	/* PBAL  - PPort at 0x378		  */
	{ 0x43, 0x03, 0x00 },	/* PBAH  - PPort at 0x378		  */
	{ 0x44, 0x01, 0xfe },	/* S1BAL - SCC1 at 0x3f8		  */
	{ 0x45, 0x03, 0x00 },	/* S1BAH - SCC1 at 0x3f8		  */
	{ 0x46, 0x01, 0xbe },	/* S2BAL - SCC2 at 0x2f8		  */
	{ 0x47, 0x03, 0x00 },	/* S2BAH - SCC2 at 0x2f8		  */
	{ 0x48, 0x01, 0xfc },	/* FBAL  - FDC at 0x3f0			  */
	{ 0x49, 0x03, 0x00 },	/* FBAH  - FDC at 0x3f0			  */
	{ 0x4c, 0x00, 0x80 },	/* SIRQ1 - DRQ3				  */
	{ 0x4d, 0x00, 0x00 },	/* SIRQ2 - 				  */
	{ 0x4e, 0x00, 0x80 },	/* SIRQ3 - PNF				  */
	{ 0x4f, 0xc0, 0x00 },	/* PNP3  - SCC2 DMA disabled		  */
	{ 0x50, 0xf0, 0x01 },	/* SCF3  - DACK3			  */
	{ 0x00, 0xc0, 0x0f }
};

static const int base_addrs[] = { 0x398, 0x279, 0x000 };
static int config_port;

static void modify_reg(char reg, char mask, char val)
{
	char old_v;

	outb(reg, config_port);
	old_v = inb(config_port + 1);

	old_v &= mask;
	val &= ~mask;

	outb(reg, config_port);
	outb(old_v | val, config_port + 1);
}

void init_87338(void)
{
	int i;

	for (i = 0; base_addrs[i]; i++)
		if (inb(base_addrs[i]) == 0x88 &&
		    inb(base_addrs[i]) == 0x00)
			break;

	if (!base_addrs[i])
		return;

	config_port = base_addrs[i];

	printf("Initialising 87338 at 0x%x\n", config_port);

	for (i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++) {
		modify_reg(regs[i].reg, regs[i].mask, regs[i].val);

		if (regs[i].reg == 0x51 && regs[i].val & 4) {
			outb(0x51, config_port);

			while ((inb(config_port + 1) & 8) == 0);
		}
	}
}
