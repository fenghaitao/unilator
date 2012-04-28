
#define sti()							\
	do {							\
		unsigned int dummy;				\
		__asm__ __volatile__(				\
		"mrs	%0, cpsr\n\t"				\
		"orr	%0, %0, #1 << 7\n\t"			\
		"msr	cpsr, %0" : "=r" (dummy) : : "memory");	\
	} while(0)

#define cli()							\
	do {							\
		unsigned int dummy;				\
		__asm__ __volatile__(				\
		"mrs	%0, cpsr\n\t"				\
		"bic	%0, %0, #1 << 7\n\t"			\
		"msr	cpsr, %0" : "=r" (dummy) : : "memory");	\
	} while(0)

