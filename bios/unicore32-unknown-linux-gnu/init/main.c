#include <stdlib.h>

#include <bios/config.h>
#include <bios/bootdev.h>
#include <bios/pci.h>
#include <bios/time.h>
#include <bios/stdio.h>
#include <bios/system.h>

typedef enum {
	ram_failure,
	ram_ok
} ram_test_t;

typedef enum {
	boot_failure,
	boot_single,
	boot_multiple
} boot_test_t;

extern struct bootdev boot_net;
extern struct bootdev boot_ide;
extern struct bootdev boot_scsi;

static struct bootdev *first;
static struct bootdev *bootdevs[] = {
#ifdef CONFIG_BOOT_IDE
	&boot_ide,
#endif
#ifdef CONFIG_BOOT_SCSI
	&boot_scsi,
#endif
#ifdef CONFIG_BOOT_NET
	&boot_net,
#endif
	NULL
};

extern void (*display_fn)(const char *buffer, int len);
extern void ser_msg(const char *buffer, int len);

extern unsigned long ramtest(unsigned long, unsigned long);

int ram_size = 0x02000000;

static ram_test_t ram_test(void)
{
	unsigned int ptr;
	ram_test_t ret = ram_ok;

	printf("       KB SDRAM OK");

	for (ptr = 0x100000; ptr < ram_size; ptr += 8) {
		if (ramtest(ptr, 0x55aacc33) != 0x55aacc33) {
			printf("\nMemory error detected at address 0x%08X", ptr);
			ret = ram_failure;
			break;
		}
		if ((ptr & 524280) == 524280)
			printf("\r%6d", (ptr + 4) >> 10);
	}

	return ret;
}

static boot_test_t locate_boot_device(void)
{
	struct bootdev **prev;
	int i, found = 0;

	first = NULL;
	prev = &first;

	printf("Locating bootable devices: ");

	for (i = 0; bootdevs[i]; i++) {
		if (bootdevs[i]->init() == 0) {
			printf(found ? ", %s" : "%s", bootdevs[i]->name);
			*prev = bootdevs[i];
			prev = &bootdevs[i]->next;
			found++;
		}
	}

	if (!found)
		goto none;

	printf("\n");

	return found != 1 ? boot_multiple : boot_single;

none:
	printf("none\n");

	return boot_failure;
}

void start_main(void)
{
	struct bootdev *dev;

#if 0	
	display_fn = 0;

	/*
	 * Initialise PCI sub-system
	 */
	pci_init();

	/*
	 * Initialise VGA adapter
	 */
#endif

	vga_init();

	printf("EBSA285 Linux BIOS v1.00 (c) 1998-1999 Russell King (rmk@arm.linux.org.uk)\n");

#if 0	
	/*
	 * Display PCI configuration
	 */
	pci_print_config();
	printf("\n");

	/*
	 * Initialise ISA
	 */
	init_87338();

#endif	
	/*
	 * Check integrity of RAM
	 */

	if (ram_test() == ram_failure)
		goto error;

	printf("\n");

#if 0
	/*
	 * Initialise timers
	 */
	time_init();
#endif

/* ignoring 2003-07-21*/
//	cli();

	switch (locate_boot_device()) {
	case boot_failure:
		goto error;

	case boot_multiple:
//		first = boot_select(first);

	case boot_single:
		break;
	}

	for (dev = first; dev; dev = dev->next) {
		printf("Trying %s...\n", dev->name);
		if (dev->start())
			continue;
		if (dev->load() == 0)
			break;
		dev->stop();
	}

	if (!dev)
		goto error;

	dev->stop();

	printf("Now booting image...\n");

//	sti();

	boot_kernel();
	return;

error:
	printf(" -- System Halted");
	while(1);
}
