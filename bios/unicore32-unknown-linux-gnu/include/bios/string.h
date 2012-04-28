/*
 * include/bios/string.h
 *
 * Copyright (C) 1998 Russell King
 */
#ifndef BIOS_STRING_H
#define BIOS_STRING_H

#if defined(__STDC__) && __GNUC__ < 2

extern void *memcpy(void *to, const void *from, int size);

#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

extern void *memzero(void *ptr, int size);

extern void *memset(void *ptr, int byte, int size);

extern int memeq(const void *m1, const void *m2, int size);

extern int strnlen(const char *p, int size);

#endif
