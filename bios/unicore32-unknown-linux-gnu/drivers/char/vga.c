/* debugging vga 2003-07-20 */
#include <bios/string.h>
#include <bios/pci.h>
#include <bios/stdio.h>

#include "font.h"

static volatile char *io_base;
static volatile char *charmap;
static volatile char *scrmem;
static unsigned char x, y, attrib;
static unsigned long card_base;
static unsigned short vendor, device;

#define NUM_COLS	80
#define NUM_ROWS	25

/* Write: 0x3c2  Read: 0x3cc */
#define VSync_Polarity	0x80
#define HSync_Polarity	0x40
#define OddEvenPage	0x20
#define DisableVideo	0x10
#define ClockSel_480	0x0c
#define ClockSel_350	0x08
#define ClockSel_400	0x04
#define EnableRAM	0x02
#define IO_Colour	0x01

#define FN_ATT	0
#define FN_GRPH	1
#define FN_CRTC	2
#define FN_SEQ	3
#define FN_DLUT	4
#define FN_MISC	5
#define FN_CMAP	6
#define FN_CARD	7
#define FN_END	8

typedef struct {
	char fn;
	char reg;
	char val;
} mux_reg_t;

typedef struct {
	unsigned short vendor;
	unsigned short device;
	const mux_reg_t *mux;
} card_mux_t;

static const mux_reg_t s3trio64_card[] = {
	{ FN_CRTC, 0x38, 0x48 },	/* Unlock 1				*/
	{ FN_CRTC, 0x39, 0xa5 },	/* Unlock 2				*/
	{ FN_CRTC, 0x31, 0x05 },
	{ FN_CRTC, 0x32, 0x40 },
	{ FN_CRTC, 0x35, 0x00 },
	{ FN_CRTC, 0x36, 0x92 },	/* 2MB, enable VGA bios			*/
	{ FN_CRTC, 0x3B, 0x5a },
	{ FN_CRTC, 0x40, 0x00 },
	{ FN_CRTC, 0x53, 0x00 },
	{ FN_CRTC, 0x54, 0x58 },
	{ FN_CRTC, 0x58, 0x00 },
	{ FN_CRTC, 0x60, 0x07 },
	{ FN_CRTC, 0x39, 0x5a },	/* Lock 2				*/
	{ FN_CRTC, 0x38, 0x00 },	/* Lock 1				*/
	{ FN_END,  0x00, 0x00 }
};

static const card_mux_t cards[] = {
	{ 0x5333, 0x8901, s3trio64_card },
	{ 0x5333, 0x8880, s3trio64_card },
	{ 0x0000, 0x0000, NULL },
};

/*
 * A 'programme' of writes to set up the video card
 */
static const mux_reg_t list[] = {
	{ FN_SEQ , 0x00, 0x01 },	/* Seq Reset register (seq reset)	*/
	{ FN_MISC, 0x00, 0x67 },
	{ FN_SEQ , 0x01, 0x00 },	/* Clock mode				*/
	{ FN_SEQ , 0x02, 0x04 },	/* Colour plane write enable		*/
	{ FN_SEQ , 0x03, 0x00 },	/* Character gen select			*/
	{ FN_SEQ , 0x04, 0x07 },	/* Memory mode register			*/
	{ FN_SEQ , 0x00, 0x03 },	/* Seq Reset register			*/

	{ FN_CARD, 0x00, 0x00 },

	{ FN_GRPH, 0x00, 0x00 },	/* Set/Reset data planes		*/
	{ FN_GRPH, 0x01, 0x00 },	/* Set/Reset enable planes		*/
	{ FN_GRPH, 0x02, 0x00 },	/* Colour compare			*/
	{ FN_GRPH, 0x03, 0x00 },	/* Data rotate/func select		*/
	{ FN_GRPH, 0x04, 0x02 },	/* Read plane select			*/
	{ FN_GRPH, 0x05, 0x00 },	/* Mode register			*/
	{ FN_GRPH, 0x06, 0x0c },	/* Miscellaneous			*/
	{ FN_GRPH, 0x07, 0x00 },	/* Colour don't care plane		*/
	{ FN_GRPH, 0x08, 0xff },	/* bitmask				*/

	{ FN_CMAP, 0x00, 0x00 },	/* Load char map			*/

	{ FN_SEQ , 0x00, 0x01 },	/* Seq Reset register (seq reset)	*/
	{ FN_SEQ , 0x01, 0x00 },	/* Seq Reset register (seq reset)	*/
	{ FN_SEQ , 0x02, 0x03 },	/* Colour plane write enable		*/
	{ FN_SEQ , 0x03, 0x00 },	/* Seq Reset register (seq reset)	*/
	{ FN_SEQ , 0x04, 0x02 },	/* Memory mode register			*/
	{ FN_SEQ , 0x00, 0x03 },	/* Seq Reset register (seq reset)	*/

	{ FN_GRPH, 0x04, 0x00 },	/* Read plane select			*/
	{ FN_GRPH, 0x05, 0x10 },	/* Mode register			*/
	{ FN_GRPH, 0x06, 0x0e },	/* Miscellaneous			*/

	{ FN_CRTC, 0x00, 0x5f },	/* Horiz total				*/
	{ FN_CRTC, 0x01, 0x4f },	/* Horiz displayed total		*/
	{ FN_CRTC, 0x02, 0x50 },	/* Horiz blank start			*/
	{ FN_CRTC, 0x03, 0x82 },	/* Horiz blank end			*/
	{ FN_CRTC, 0x04, 0x55 },	/* Horiz retrace start			*/
	{ FN_CRTC, 0x05, 0x81 },	/* Horiz retrace end			*/
	{ FN_CRTC, 0x06, 0xbf },	/* Vert total				*/
	{ FN_CRTC, 0x07, 0x1f },	/* Overflow register			*/
	{ FN_CRTC, 0x08, 0x00 },	/* Preset row scan			*/
	{ FN_CRTC, 0x09, 0x4f },	/* Max scan line			*/
	{ FN_CRTC, 0x0a, 0x0d },	/* Cursor scanline start		*/
	{ FN_CRTC, 0x0b, 0x0e },	/* Cursor scanline end			*/
	{ FN_CRTC, 0x0c, 0x00 },	/* Start address high			*/
	{ FN_CRTC, 0x0d, 0x00 },	/* Start address low			*/
	{ FN_CRTC, 0x0e, 0x00 },	/* Cursor high				*/
	{ FN_CRTC, 0x0f, 0x00 },	/* Cursor low				*/
	{ FN_CRTC, 0x10, 0x9c },	/* Vert retrace start			*/
	{ FN_CRTC, 0x11, 0x8e },	/* Vert retrace end			*/
	{ FN_CRTC, 0x12, 0x8f },	/* Vert display enable end		*/
	{ FN_CRTC, 0x13, 0x28 },	/* Logical screen width			*/
	{ FN_CRTC, 0x14, 0x1f },	/* Underline location			*/
	{ FN_CRTC, 0x15, 0x96 },	/* Vertical blanking start		*/
	{ FN_CRTC, 0x16, 0xb9 },	/* Vertical blanking end		*/
	{ FN_CRTC, 0x17, 0xa3 },	/* mode control				*/
	{ FN_CRTC, 0x18, 0xff },	/* Compare register			*/

	{ FN_ATT , 0x00, 0x00 },	/* Attrib Palette 0			*/
	{ FN_ATT , 0x01, 0x01 },	
	{ FN_ATT , 0x02, 0x02 },	
	{ FN_ATT , 0x03, 0x03 },	
	{ FN_ATT , 0x04, 0x04 },	
	{ FN_ATT , 0x05, 0x05 },	
	{ FN_ATT , 0x06, 0x06 },	
	{ FN_ATT , 0x07, 0x07 },	
	{ FN_ATT , 0x08, 0x08 },	
	{ FN_ATT , 0x09, 0x09 },	
	{ FN_ATT , 0x0a, 0x0a },	
	{ FN_ATT , 0x0b, 0x0b },	
	{ FN_ATT , 0x0c, 0x0c },	
	{ FN_ATT , 0x0d, 0x0d },	
	{ FN_ATT , 0x0e, 0x0e },	
	{ FN_ATT , 0x0f, 0x0f },	/* Attrib Palette 15			*/
	{ FN_ATT , 0x10, 0x08 },	/* Mode control				*/
	{ FN_ATT , 0x11, 0x00 },	/* Screen border			*/
	{ FN_ATT , 0x12, 0x0f },	/* Colour plane enable			*/
	{ FN_ATT , 0x13, 0x08 },	/* Horizontal panning			*/
	{ FN_ATT , 0x14, 0x00 },	/* Colour select			*/

	{ FN_DLUT, 0x00, 0x00 },	/* Load DAC LUTs			*/
	{ FN_END , 0x00, 0x00 }
};
	
static const char DAC[] = {
	 0,  0,  0,   0,  0, 42,   0, 42,  0,   0, 42, 42,
	42,  0,  0,  42,  0, 42,  42, 42,  0,  42, 42, 42,
	21, 21, 21,  21, 21, 63,  21, 63, 21,  21, 63, 63,
	63, 21, 21,  63, 21, 63,  63, 63, 21,  63, 63, 63
};

static inline void attrib_write(int reg, int val)
{
	io_base[0x3c0] = reg;
	io_base[0x3c0] = val;
}

static inline void crtc_write(int reg, int val)
{
	io_base[0x3d4] = reg;
	io_base[0x3d5] = val;
}

static inline void seq_write(int reg, int val)
{
	io_base[0x3c4] = reg;
	io_base[0x3c5] = val;
}

static inline void grph_write(int reg, int val)
{
	io_base[0x3ce] = reg;
	io_base[0x3cf] = val;
}

void vga_setcursor(void)
{
	int loc = y * NUM_COLS + x;

	crtc_write(0x0e, loc >> 8);
	crtc_write(0x0f, loc);
}

static void vga_load_regs(const mux_reg_t *reg, unsigned short vendor, unsigned short device)
{
	int c, r, i;

	while (reg->fn != FN_END) {
		switch (reg->fn) {
		case FN_ATT:
			attrib_write(reg->reg, reg->val);
			break;

		case FN_GRPH:
			grph_write(reg->reg, reg->val);
			break;

		case FN_CRTC:
			crtc_write(reg->reg, reg->val);
			break;

		case FN_SEQ:
			seq_write(reg->reg, reg->val);
			break;

		case FN_DLUT:
			io_base[0x3c8] = 0;
			for (i = 0; i < sizeof(DAC); i++)
				io_base[0x3c9] = DAC[i];
			break;

		case FN_MISC:
			io_base[0x3c2] = reg->val;
			break;

		case FN_CMAP:
			for (c = 0; c < 256; c++)
				for (r = 0; r < 32; r++)
					if (r < 8)
						charmap[(c << 5) + r] = cmap_80[c][r];
					else
						charmap[(c << 5) + r] = 0;
			break;

		case FN_CARD:
			for (i = 0; cards[i].vendor; i++)
				if (cards[i].vendor == vendor &&
				    cards[i].device == device)
					vga_load_regs(cards[i].mux, 0, 0);
			break;
		}
		reg++;
	}
}

static void vga_clear(void)
{
	int i;

	for (i = 0; i < NUM_ROWS * NUM_COLS * 2; i += 4) {
		*(unsigned int *)(scrmem+i)=0x07200720;
	}
}

static void vga_write_string(const char *buffer, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		switch (buffer[i]) {
		case 8:
			if (x > 0)
				x -= 1;
			break;

		case 9:
			x = (x | 7) + 1;
			break;

		case '\n':
			y += 1;
			x = 0;
			break;

		case '\r':
			x = 0;
			break;

		case 12:
			vga_clear();
			break;

		case 31 ... 255:
			scrmem[(y * NUM_COLS + x) * 2] = buffer[i];
			scrmem[(y * NUM_COLS + x) * 2 + 1] = attrib;
			x += 1;
			break;
		}

		if (x >= NUM_COLS) {
			x = 0;
			y += 1;
		}
		if (y >= NUM_ROWS)
			y = 0;
//y = NUM_ROWS;
	}
}

void vga_print(const char *buffer, int len)
{
	vga_write_string(buffer, len);
	vga_setcursor();
}

void vga_init(void)
{
#if 0	
	card_base = pci_lookupclass(0x300);
	if (!card_base)
		card_base = pci_lookupclass(0x0001);
	if (!card_base)
		return;
#endif
	x = 0;
	y = 0;
	attrib = 7;
#if 0
	vendor = pci_read_config_word(card_base, PCI_VENDOR_ID);
	device = pci_read_config_word(card_base, PCI_DEVICE_ID);
#endif
	io_base = (volatile char *)0x00000000;
	charmap = (volatile char *)0x000b8000;
	scrmem  = (volatile char *)0x000b8000;

	io_base[0x3c3] = 1;

	vendor = 0;
	device = 0;
	
	vga_load_regs(list, vendor, device);

	io_base[0x3c0] = 0x20;
	io_base[0x3c6] = 0xff;

	vga_clear();
	vga_setcursor();

	display_fn = vga_print;
}

void con_get_params(int *xp, int *yp, int *colsp, int *rowsp)
{
	*xp = x;
	*yp = y;
	*colsp = NUM_COLS;
	*rowsp = NUM_ROWS;
}
