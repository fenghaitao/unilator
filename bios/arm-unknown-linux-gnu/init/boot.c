#include <bios/boot.h>
#include <bios/string.h>
#include <asm/setup.h>

#define LOAD_ADDR	0x108000

#define ARCH_TYPE 4
#define CMD_LINE ""

static struct param_struct *params = (struct param_struct *)0x100100;
extern int ram_size;

u32	root_dev;
u32	root_flags;
u32	load_addr = LOAD_ADDR;

void boot_kernel(void)
{
	memset(params, 0, sizeof(*params));

	params->u1.s.page_size		= 4096;
	params->u1.s.nr_pages		= ram_size / 4096;
	params->u1.s.rootdev		= root_dev;
	params->u1.s.flags		= root_flags;

	con_get_params(&params->u1.s.video_x,
		       &params->u1.s.video_y,
		       &params->u1.s.video_num_cols,
		       &params->u1.s.video_num_rows);

	strcpy(params->commandline, CMD_LINE);
	/* debugging kernel fht 2003-10-22 */
	//__asm__("msr spsr,r0");	
	((void (*)(int,int))load_addr)(0,ARCH_TYPE);
}


