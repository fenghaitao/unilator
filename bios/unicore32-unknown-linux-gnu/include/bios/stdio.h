#include <stdarg.h>

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

/*
 * function to be called to display printf output
 */
extern void (*display_fn)(const char *buffer, int len);

