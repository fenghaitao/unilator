#include <bios/netdev.h>
#include <bios/boot.h>
#include <bios/time.h>
#include <bios/config.h>
#include <bios/bootdev.h>
#include <bios/netdev.h>
#include <bios/pci.h>
#include <bios/stdio.h>

#include "ide.h"

#define NR_DRIVES	2

#define PIO_PIO0	0
#define PIO_PIO1	7
#define PIO_PIO2	12
#define PIO_PIO3	14
#define PIO_PIO4	16

#define DMA_NONE	0
#define DMA_DMA0	1
#define DMA_DMA1	2
#define	DMA_DMA2	3
#define DMA_UDMA0	3
#define DMA_UDMA1	4
#define DMA_UDMA2	5

#define wait_cs(i)	

static struct ide_hw	hw[NR_DRIVES/2];
static struct ide_drive	drives[NR_DRIVES];

/*
 * figure out which status register to use for probing
 */
static inline int ide_get_status_reg(struct ide_drive *drive)
{
	u8 s, a;

	a = ide_inb(drive, IDE_ALTSTATUS);
	s = ide_inb(drive, IDE_STATUS);

	return ((a ^ s) & ~INDEX_STAT) ? IDE_STATUS : IDE_ALTSTATUS;
}

/*
 * Send a command to the drive and wait for BUSY status
 */
static inline int ide_send_command(struct ide_drive *drive, u8 cmd, int timeout, int hd_status)
{
	ide_outb(drive, cmd, IDE_COMMAND);

	timeout += centisecs;

	do {
		if (timeout < centisecs)
			return 1;
		wait_cs(5);
	} while (ide_inb(drive, hd_status) & BUSY_STAT);

	wait_cs(5);

	return 0;
}

/*
 * check whether status is ok
 */
static inline int ide_status_ok(struct ide_drive *drive, int ok, int bad)
{
	u8 s;

	s = ide_inb(drive, IDE_STATUS);

	return (s & (ok | bad)) == ok;
}

/*
 * select a drive
 */
static inline int ide_select_drive(struct ide_drive *drive)
{
	ide_outb(drive, drive->select, IDE_SELECT);

	wait_cs(5);

	return ide_inb(drive, IDE_SELECT) == drive->select;
}

/*
 * read some data from the drive
 */
static void ide_input_data(struct ide_drive *drive, u8 *data, int len)
{
	unsigned short *s = (unsigned short *)data;

	while (len > 0) {
		len -= 2;
		*s++ = ide_inw(drive, IDE_DATA);
	}
}

/*
 * convert a drive string to our endian-ness
 */
static void ide_fixstring(u8 *s, int size, int bswap)
{
	u8 *p = s, *end = &s[size & ~1];

	if (bswap) {
		for (p = end; p != s;) {
			unsigned short *pp = (unsigned short *)(p -= 2);
			*pp = htons(*pp);
		}
	}

	while (s != end && *s == ' ')
		++s;

	while (s != end && *s) {
		if (*s++ != ' ' || (s != end && *s && *s != ' '))
			*p++ = *(s - 1);
	}

	/* wipe out trailing garbage */
	while (p != end)
		*p++ = '\0';
}

static int lba_ok(struct ide_drive *drive, struct hd_driveid *id)
{
	u32 lba_sects = id->lba_capacity;
	u32 chs_sects = id->cyls * id->heads * id->sectors;
	u32 chs_10_pc = chs_sects / 10;

	if (id->cyls == 16383 && id->heads == 16 && id->sectors == 63 &&
	    lba_sects > chs_sects) {
		id->cyls = lba_sects / (16 * 63);
		return 1;
	}

	if ((lba_sects - chs_sects) < chs_10_pc)
		return 1;

	lba_sects = (lba_sects << 16) | (lba_sects >> 16);
	if ((lba_sects - chs_sects) < chs_10_pc) {
		id->lba_capacity = lba_sects;
		return 1;
	}
	return 0;
}

static int idx(unsigned int val, unsigned int *vals)
{
	int i;

	for (i = 0; vals[i]; i++)
		if (val >= vals[i])
			break;

	return i;
}

#if 0
static void get_speed_params(struct ide_drive *drive, struct hd_driveid *id)
{
	int eide_pio_cyc_time, c;

	c = pci_read_config_byte(drive->hw->pci_base, drive->ifnr ? 0x6a : 0x62);

	if (c & 0x40)
		drive->iordy = 1;

	if (drive->iordy)
		eide_pio_cyc_time = id->eide_pio_iordy;
	else
		eide_pio_cyc_time = id->eide_pio;

	drive->dma_speed = DMA_NONE;
	drive->pio_speed = 0;
	drive->dma = 0;

	/*
	 * Try UDMA
	 */
	if (id->field_valid & 4) {
		if (id->dma_ultra & 4) {	/* UDMA mode 2 */
			drive->dma_speed = DMA_UDMA2;
			drive->dma = 2;
		} else
		if (id->dma_ultra & 2) {	/* UDMA mode 1 */
			drive->dma_speed = DMA_UDMA1;
			drive->dma = 2;
		} else
		if (id->dma_ultra & 1) {	/* UDMA mode 0 */
			drive->dma_speed = DMA_UDMA0;
			drive->dma = 2;
		}
	}

	/*
	 * Try EIDE DMA/timed PIO
	 */
	if (id->field_valid & 2) {
		/*
		 * EIDE DMA
		 */
		if (drive->dma == 0 && id->eide_dma_time != 0 && id->eide_dma_time <= 180) {
			static unsigned int eide_times[] = { 150, 120, 90, 60, 0 };
			drive->dma = 1;

			drive->dma_speed = DMA_DMA0 + idx(id->eide_dma_time, eide_times);
		}
		/*
		 * EIDE PIO
		 */
		if (eide_pio_cyc_time != 0) {
			static unsigned int eide_pio_times[] =
				{ 570, 540, 510, 480, 450, 420, 390, 360, 330,
				  300, 270, 240, 210, 180, 150, 120, 90, 60, 0 };

			drive->pio_speed = idx(eide_pio_cyc_time, eide_pio_times);
			return;
		}
	}

	switch (id->eide_pio_modes >> 8) {
	default:/* mode 4 - 210ns */
		drive->pio_speed = 13;
		break;

	case 1:	/* mode 3 - 390ns */
		drive->pio_speed = 7;
		break;

	case 0:
		drive->pio_speed = 0;
		break;
	}
}

static int set_drive_features(struct ide_drive *drive)
{
	int mode = 0;
	int failed = 0;

	switch (drive->dma) {
	case 2:	/* UDMA */
		mode = 0x40 | (drive->dma_speed - DMA_UDMA0);
		break;

	case 1:	/* DMA */
		if (drive->dma_speed == DMA_DMA1)
			mode = 0x21;
		else if (drive->dma_speed > DMA_DMA1)
			mode = 0x22;
		break;

	case 0: /* PIO */
		switch (drive->pio_speed) {
		case 16 ... 18:
			mode = 0x0c;	/* mode4 */
			break;

		case 14 ... 15:
			mode |= 0x0b;	/* mode3 */
			break;

		default:
			break;
		}
		break;
	}

	if (mode) {
		if (!ide_select_drive(drive))
			failed = 1;

		ide_outb(drive, mode, IDE_NSECTOR);
		ide_outb(drive, 0x03, IDE_FEATURE);

		wait_cs(1);

		if (!failed && ide_send_command(drive, WIN_SETFEATURES, 2 * 100, IDE_STATUS))
			failed = 1;

		if (!failed && !ide_status_ok(drive, 0, ERR_STAT|DRQ_STAT|BUSY_STAT))
			failed = 1;
	}

	return failed;
}

static void set_program_if(struct ide_drive *drive)
{
	static const struct { u8 c:4, b:3; } dma_regs[] = {
		{ 15, 7 },	/* NONE			*/
		{  4, 4 },	/* DMA0		180ns	*/
		{  4, 3 },	/* DMA1		150ns	*/
		{  3, 3 },	/* UDMA0 / DMA2	120ns	*/
		{  2, 2 },	/* UDMA1	 90ns	*/
		{  1, 1 }	/* UDMA2	 60ns	*/
	};

	static const struct { u8 a:4, b:5; } pio_regs[] = {
		{ 13, 19 },	/* PIO0		600ns	*/
		{ 12, 18 },	/* PIO0		570ns	*/
		{ 11, 17 },	/* PIO0		540ns	*/
		{ 10, 16 },	/* PIO0		510ns	*/
		{  9, 15 },	/* PIO0		480ns	*/
		{  8, 14 },	/* PIO0		450ns	*/
		{  7, 13 },	/* PIO0		420ns	*/
		{  7, 12 },	/* PIO1		390ns	*/
		{  6, 11 },	/* PIO1		360ns	*/
		{  5, 10 },	/* PIO1		330ns	*/
		{  4,  9 },	/* PIO1		300ns	*/
		{  3,  8 },	/* PIO1		270ns	*/
		{  3,  7 },	/* PIO2		240ns	*/
		{  2,  6 },	/* PIO2		210ns	*/
		{  2,  5 },	/* PIO3		180ns	*/
		{  1,  4 },	/* PIO3		150ns	*/
		{  0,  3 },	/* PIO4		120ns	*/
		{  0,  2 },	/* PIO4		 90ns	*/
		{  0,  1 }	/* PIO4		 60ns	*/
	};
	unsigned long pci_off;
	u8 a, b, c;

	pci_off = (drive->ifnr ? 0x68 : 0x60) + (drive->slave ? 4 : 0);

	a = 0xd0 | pio_regs[drive->pio_speed].a;
	b = pio_regs[drive->pio_speed].b;
	c = 0;

	if (drive->iordy)
		a |= 0x20;

	if (drive->dma && drive->dma_speed <= DMA_UDMA2) {
		b |= dma_regs[drive->dma_speed].b << 5;
		c |= dma_regs[drive->dma_speed].c;
	} else {
		b |= 0xe0;
		c |= 0x0f;
	}

	pci_write_config_byte(drive->hw->pci_base, pci_off + 0, a);
	pci_write_config_byte(drive->hw->pci_base, pci_off + 1, b);
	pci_write_config_byte(drive->hw->pci_base, pci_off + 2, c);

	if (drive->dma) {
		c = ide_in_dma(drive, 2);

		if (drive->slave)
			c |= 0x40;
		else
			c |= 0x20;

		ide_out_dma(c, drive, 2);
	}
}

#endif 

/*
 * try to issue an IDENTIFY command
 */
static int ide_identify(struct ide_drive *drive, u8 cmd)
{
	u8 idbuf[512];
	struct hd_driveid *id = (struct hd_driveid *)idbuf;
	int hd_status, rc;

	hd_status = ide_get_status_reg(drive);

	if (ide_send_command(drive, cmd, 30 * 100, hd_status))
		return 1;

	if (ide_status_ok(drive, DRQ_STAT, BAD_R_STAT)) {
		u32 capacity, check;

		ide_input_data(drive, idbuf, 512);

		ide_fixstring(id->model, sizeof(id->model), 1);

		id->model[sizeof(id->model)-1] = '\0';
		printf("%-36s, ", id->model);

		drive->sect = id->sectors;
		drive->head = id->heads;
		drive->cyl  = id->cyls;

		if ((id->field_valid & 1) && id->cur_cyls && id->cur_heads &&
		    id->cur_heads <= 16 && id->cur_sectors) {
			drive->sect = id->cur_sectors;
			drive->head = id->cur_heads;
			drive->cyl  = id->cur_cyls;

			capacity = drive->cyl * drive->head * drive->sect;
			check = id->cur_capacity0 << 16 | id->cur_capacity1;

			if (check == capacity) {
				id->cur_capacity0 = capacity;
				id->cur_capacity1 = capacity >> 16;
			}
		}

		capacity = drive->cyl * drive->head * drive->sect;

		drive->lba = 0;
		drive->select &= 0xbf;
		if ((id->capability & 2) && lba_ok(drive, id)) {
			if (id->lba_capacity >= capacity) {
				drive->cyl = id->lba_capacity / (drive->head * drive->sect);
				drive->lba = 1;
				drive->select |= 0x40;
				capacity = id->lba_capacity;
			}
		}

#if 0		
		ide_out_dma(ide_in_dma(drive, 0) & ~1, drive, 0);
		get_speed_params(drive, id);
		if (!set_drive_features(drive))
			set_program_if(drive);
#endif
		printf("%ldMB, %sCHS=%d/%d/%d\n",
			capacity /2048,
			drive->lba ? "LBA, " : "",
			drive->cyl, drive->head, drive->sect);
		rc = 0;
	} else
		rc = 2;

	return rc;
}

/*
 * try to detect a drive
 */
static int ide_drive_probe(struct ide_drive *drive)
{
	int rc;

	if (!ide_select_drive(drive))
		return 3;

	if (ide_status_ok(drive, READY_STAT, BUSY_STAT)) {
		rc = ide_identify(drive, WIN_IDENTIFY);
		if (rc)
			rc = ide_identify(drive, WIN_IDENTIFY);
		ide_inb(drive, IDE_STATUS);
	} else
		rc = 3;
	return rc;
}

static int ide_read_sectors(struct ide_drive *drive, u32 sector, void *buffer, int len)
{
	u8 sect, lcyl, hcyl, sel, *buf = (u8 *)buffer;
	unsigned int target;
	int status, rc = -1;

	if (len & 511)
		return -1;

	len = len >> 9;

	if (!ide_select_drive(drive))
		return -1;

	wait_cs(1);

	if (!ide_status_ok(drive, READY_STAT, BUSY_STAT|DRQ_STAT)) {
		printf("hd%c: drive not ready for command\n", drive->id);
		return -1;
	}

	ide_outb(drive, 8, IDE_CONTROL);
	ide_outb(drive, len, IDE_NSECTOR);

	if (drive->lba) {
		sect = sector;
		lcyl = sector >> 8;
		hcyl = sector >> 16;
		sel  = (sector >> 24) & 15;
	} else {
		unsigned int cyl, track;

		sect  = sector % drive->sect + 1;
		track = sector / drive->sect;
		sel   = track % drive->head;
		cyl   = track / drive->head;
		lcyl  = cyl;
		hcyl  = cyl >> 8;
	}

	ide_outb(drive, sect, IDE_SECTOR);
	ide_outb(drive, lcyl, IDE_LCYL);
	ide_outb(drive, hcyl, IDE_HCYL);
	ide_outb(drive, sel | drive->select, IDE_SELECT);
	ide_outb(drive, WIN_READ, IDE_COMMAND);

	wait_cs(1);

	target = centisecs + 2000;
	do {
		status = ide_inb(drive, IDE_STATUS);

		if (status & BUSY_STAT)
		       continue;

		if (status & ERR_STAT)
			break;

		if (status & DRQ_STAT) {
			target = centisecs + 100;
			ide_input_data(drive, buf, 512);
			buf += 512;
			len -= 1;
		}
	} while (target > centisecs && len);

	if (!len)
		rc = 0;

	return rc;
}

static int ide_probe(void)
{
	unsigned long base;
	int i;
	
	base = 0x1f0;
	for(i = 0; i < 8; i++)
	hw[0].regs[i] = base + i;
	
	hw[0].regs[8] = 0x3f6;
#if 0	
	hw[0].dma_base = pci_read_config_long(pci_base, PCI_BASE4) & ~3;
	hw[0].pci_base = pci_base;
	
	i = pci_io_read_byte(hw[0].dma_base + 2);
	pci_io_write_byte(i & ~0x60, hw[0].dma_base + 2);
	
	base = 0x170;
	for(i = 0; i < 8; i++)
	hw[1].regs[i] = base + i;
	
	hw[0].regs[8] = 0x376;
	
	hw[1].dma_base = (pci_read_config_long(pci_base, PCI_BASE4) & ~3) + 8;
	hw[1].pci_base = pci_base;
	
	i = pci_io_read_byte(hw[1].dma_base + 2);
	pci_io_write_byte(i & ~0x60, hw[1].dma_base + 2);
#endif	
	return 0 ;
}

static int ide_start(void)
{
	int i, present = 0;

	for (i = 0; i < NR_DRIVES; i++) {
		drives[i].hw      = &hw[i >> 1];
		drives[i].ifnr    = i >> 1;
		drives[i].slave   = i & 1;
		drives[i].id      = 'a' + i;
		drives[i].select  = 0xa0 | ((i & 1) << 4);
		drives[i].lba     = 0;
		drives[i].present = 0;
		drives[i].iordy   = 0;
	}

	for (i = 0; i < NR_DRIVES; i++) {
		printf(" hd%c: ", drives[i].id);
		if (ide_drive_probe(drives + i))
			printf("not present\n");
		else
			present += 1;
	}

	return present ? 0 : -1;
}

static void dump_sector(u8 *buffer)
{
	int i, j;

	for (i = 0; i < 512; i += 16) {
		debug_printf("%03X  ", i);

		for (j = i; j < i + 16; j++)
			debug_printf("%02X ", buffer[j]);

		debug_printf("  ");

		for (j = i; j < i + 16; j++)
			debug_printf("%c", (buffer[j] < 32 || buffer[j] > 127) ? '.' : buffer[j]);
		debug_printf("\n");
	}
}

static u32 part_getbootstart(struct ide_drive *drive)
{
	u8 sector[512], *p;
	int nr;

	if (ide_read_sectors(drive, 0, sector, 512))
		return 0;

	if ((sector[510] != 0x55 || sector[511] != 0xaa) &&
	    (sector[510] != 'R' || sector[511] != 'K'))
		return 0;

	for (p = sector + 0x1be, nr = 0; nr < 4; p += 16, nr++)
		if (p[0] == 0x80)
			return p[8] | p[9] << 8 | p[10] << 16 | p[11] << 24;
	return 0;
}

struct map {
	u32	magic;
	u32	block_sz;
	struct {
		u32	off;
		u32	len;
	} map[63];
};

static int read_mapped_file(struct ide_drive *drive, u32 part_start, struct map *map, u8 *ptr)
{
	u8 *s = ptr;
	int i,sectors;

	if (map->magic != 0xc53a4b2d)
		return -1;

	dump_sector((u8 *)map);
	
	for (i = 0; i < 63 && map->map[i].off; i++) {
		u32 start;

		start = part_start + map->map[i].off * map->block_sz / 512;
		
		sectors = map->map[i].len;
		do{
			if (ide_read_sectors(drive, start, ptr, (sectors>128?128:sectors) * map->block_sz))
				return -1;

			ptr += (sectors>128?128:sectors) * map->block_sz;

			sectors-=128;
		
		
		}while (sectors > 0);
	}

	dump_sector(s);

	return 0;
}

static int ide_load(void)
{
	struct ide_drive *drive;
	struct map map;

	u32 part_start;
	int i,block_size=40;

	for (i = 0; i < 4; i++) {
		drive = drives + i;

		part_start = part_getbootstart(drive);

		if (part_start)
			break;
	}
	
	if (part_start == 0)
		return -1;

	if (ide_read_sectors(drive, part_start, &map, sizeof(map)))
		return -1;

	memcpy((void *)&map,(void *)((unsigned char *)&map+0x60),block_size);

	if (read_mapped_file(drive, part_start, &map, (u8 *)load_addr))
		return -1;

#if 0	
	/* If J17 P13-14 is made, then don't boot off the HD
	 */
	if (!((*(u8 *)0x40012000) & 0x10))
		return -1;
#endif
	
	root_flags = 1;
	root_dev = 0x301;

	return 0;
}

static int ide_stop(void)
{
	return 0;
}

struct bootdev boot_ide = {
	"ide",

	ide_probe,
	ide_start,
	ide_load,
	ide_stop
};
