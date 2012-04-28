static const char pnp_init_key[] = {
	0x00, 0x00,
	0x6a, 0xb5, 0xda, 0xed, 0xf6, 0xfb, 0x7d, 0xbe,
	0xdf, 0x6f, 0x37, 0x1b, 0x0d, 0x86, 0xc3, 0x61,
	0xb0, 0x58, 0x2c, 0x16, 0x8b, 0x45, 0xa2, 0xd1,
	0xe8, 0x74, 0x3a, 0x9d, 0xce, 0xe7, 0x73, 0x39
};

static void pnp_config(int data)
{
	outb(0x02, PNP_ADDRESS);
	outb(data, PNP_WRDATA);
}

static void pnp_wake(int csn)
{
	outb(0x03, PNP_ADDRESS);
	outb(csn, PNP_WRDATA);
}

static void pnp_set_rddata(int port)
{
	outb(0x00, PNP_ADDRESS);
	outb(port, PNP_WRDATA);
}

static void pnp_init(void)
{
	int i;

	/*
	 * Force wait-for-key
	 */
	pnp_config(0x02);

	udelay(2);

	/*
	 * Send key
	 */
	for (i = 0; i < sizeof(pnp_init_key); i++)
		outb(pnp_init_key[i], PNP_ADDRESS);

	/*
	 * Reset CSN
	 */
	pnp_config(0x04);

	/*
	 * Wake board 0
	 */
	pnp_wake(0);

	/*
	 * read port at 0x203
	 */
	pnp_rddata(0x80);

	udelay(1);

	/*
	 * Serial isolation mode
	 */
	outb(0x01, PNP_ADDRESS);

	udelay(2);

	for (i = 0; i < nr_boards; i++) {
		/*
		 * Wake selected device
		 */
		pnp_wake(i);
	}
}
