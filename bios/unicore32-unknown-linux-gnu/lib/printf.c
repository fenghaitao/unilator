#include <bios/stdio.h>
#include <stdarg.h>

void (*display_fn)(const char *buffer, int len);

int printf(const char *fmt, ...)
{
	char buffer[1024];
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsprintf(buffer, fmt, ap);
	va_end(ap);

	if (display_fn)
		display_fn(buffer, len);

	return len;
}

