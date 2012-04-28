
#define IDE_DATA 	(0)
#define IDE_ERROR	(1)	/* r */
#define IDE_FEATURE	(1)	/* w */
#define IDE_NSECTOR	(2)
#define IDE_SECTOR	(3)
#define IDE_LCYL 	(4)
#define IDE_HCYL 	(5)
#define IDE_SELECT	(6)
#define IDE_STATUS	(7)	/* r */
#define IDE_COMMAND	(7)	/* w */
#define IDE_ALTSTATUS	(8)	/* r */
#define IDE_CONTROL	(8)	/* w */

#define ERR_STAT	0x01
#define INDEX_STAT	0x02
#define ECC_STAT	0x04	/* Corrected error */
#define DRQ_STAT	0x08
#define SEEK_STAT	0x10
#define WRERR_STAT	0x20
#define READY_STAT	0x40
#define BUSY_STAT	0x80

#define BAD_R_STAT	(BUSY_STAT | ERR_STAT)

#define WIN_IDENTIFY            0xEC    /* ask drive to identify itself */
#define WIN_READ                0x20
#define WIN_SETFEATURES		0xEF

struct hd_driveid {
	unsigned short	config; 	/* lots of obsolete bit flags */
	unsigned short	cyls;		/* "physical" cyls */
	unsigned short	reserved2;	/* reserved (word 2) */
	unsigned short	heads;		/* "physical" heads */
	unsigned short	track_bytes;	/* unformatted bytes per track */
	unsigned short	sector_bytes;	/* unformatted bytes per sector */
	unsigned short	sectors;	/* "physical" sectors per track */
	unsigned short	vendor0;	/* vendor unique */
	unsigned short	vendor1;	/* vendor unique */
	unsigned short	vendor2;	/* vendor unique */
	unsigned char	serial_no[20];	/* 0 = not_specified */
	unsigned short	buf_type;
	unsigned short	buf_size;	/* 512 byte increments; 0 = not_specified */
	unsigned short	ecc_bytes;	/* for r/w long cmds; 0 = not_specified */
	unsigned char	fw_rev[8];	/* 0 = not_specified */
	unsigned char	model[40];	/* 0 = not_specified */
	unsigned char	max_multsect;	/* 0=not_implemented */
	unsigned char	vendor3;	/* vendor unique */
	unsigned short	dword_io;	/* 0=not_implemented; 1=implemented */
	unsigned char	vendor4;	/* vendor unique */
	unsigned char	capability;	/* bits 0:DMA 1:LBA 2:IORDYsw 3:IORDYsup*/
	unsigned short	reserved50;	/* reserved (word 50) */
	unsigned char	vendor5;	/* vendor unique */
	unsigned char	tPIO;		/* 0=slow, 1=medium, 2=fast */
	unsigned char	vendor6;	/* vendor unique */
	unsigned char	tDMA;		/* 0=slow, 1=medium, 2=fast */
	unsigned short	field_valid;	/* bits 0:cur_ok 1:eide_ok */
	unsigned short	cur_cyls;	/* logical cylinders */
	unsigned short	cur_heads;	/* logical heads */
	unsigned short	cur_sectors;	/* logical sectors per track */
	unsigned short	cur_capacity0;	/* logical total sectors on drive */
	unsigned short	cur_capacity1;	/*  (2 words, misaligned int)	  */
	unsigned char	multsect;	/* current multiple sector count */
	unsigned char	multsect_valid; /* when (bit0==1) multsect is ok */
	unsigned int	lba_capacity;	/* total number of sectors */
	unsigned short	dma_1word;	/* single-word dma info */
	unsigned short	dma_mword;	/* multiple-word dma info */
	unsigned short	eide_pio_modes; /* bits 0:mode3 1:mode4 */
	unsigned short	eide_dma_min;	/* min mword dma cycle time (ns) */
	unsigned short	eide_dma_time;	/* recommended mword dma cycle time (ns) */
	unsigned short	eide_pio;	/* min cycle time (ns), no IORDY  */
	unsigned short	eide_pio_iordy; /* min cycle time (ns), with IORDY */
	unsigned short	word69;
	unsigned short	word70;
	/* HDIO_GET_IDENTITY currently returns only words 0 through 70 */
	unsigned short	word71;
	unsigned short	word72;
	unsigned short	word73;
	unsigned short	word74;
	unsigned short	word75;
	unsigned short	word76;
	unsigned short	word77;
	unsigned short	word78;
	unsigned short	word79;
	unsigned short	word80;
	unsigned short	word81;
	unsigned short	word82;
	unsigned short	word83;
	unsigned short	word84;
	unsigned short	word85;
	unsigned short	word86;
	unsigned short	word87;
	unsigned short	dma_ultra;
	unsigned short	reserved[167];
};

struct ide_hw {
	unsigned long	regs[9];
	unsigned long	dma_base;
	unsigned long	pci_base;
};

struct ide_drive {
	struct ide_hw	*hw;		/* hardware	*/
	unsigned int	sect;
	unsigned int	head;
	unsigned int	cyl;
	u8		ifnr;		/* Interface nr */
	u8		select;
	u8		dma_speed;
	u8		pio_speed;
	char		id;
	u8		dma:2;
	u8		lba:1;
	u8		present:1;
	u8		iordy:1;
	u8		slave:1;
};

#define ide_inb(d,x)	pci_io_read_byte((d)->hw->regs[(x)])
#define ide_outb(d,w,x)	pci_io_write_byte((w), (d)->hw->regs[(x)])
#define ide_inw(d,x)	pci_io_read_word((d)->hw->regs[(x)])
#define ide_outw(d,w,x)	pci_io_write_word((w), (d)->hw->regs[(x)])
#define ide_inl(d,x)	pci_io_read_long((d)->hw->regs[(x)])
#define ide_outl(d,w,x)	pci_io_write_long((w), (d)->hw->regs[(x)])

#define ide_in_dma(d,x)	pci_io_read_byte((d)->hw->dma_base)
#define ide_out_dma(w,d,x) pci_io_write_byte((w), (d)->hw->dma_base)
