#include <stdarg.h>
#include <bios/stdio.h>


void (*display_fn)(const char *buffer, int len);

static char buffer[1024];

extern void ser_msg(const char *);
extern int ser_getc(void);
extern void vga_print(const char *buffer, int len);

void debug_printf(char *fmt, ...)
{
	va_list ap;
	int len;
	
	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);
/*
	if (display_fn)
	       display_fn(buffer, len);
*/	
	/*ser_msg(buffer);*/
}

int debug_getc(void)
{
	return ser_getc();
}
