#include <bios/dec21285.h>
#include <bios/time.h>

#define LATCH ((CLOCK_TICK_RATE/16 + 50) / 100)

volatile unsigned int centisecs;
unsigned int leds = 7;

void time_interrupt(void)
{
	static int led_timer;

	csr_write_long(0, CSR_TIMER1_CLR);

	centisecs += 1;

	if (++led_timer >= 100) {
		led_timer = 0;
		leds ^= 2;
		*(unsigned long *)0x40012000 = leds;
	}
}

void wait_cs(int cs)
{
	int target = centisecs + cs;

	while (centisecs < target);
}

void time_init(void)
{
	csr_write_long(0, CSR_TIMER1_CLR);
	csr_write_long(LATCH, CSR_TIMER1_LOAD);
	csr_write_long(TIMER_CNTL_ENABLE | TIMER_CNTL_AUTORELOAD | TIMER_CNTL_DIV16, CSR_TIMER1_CNTL);
	csr_write_long(0xffffffff, CSR_IRQ_DISABLE);
	csr_write_long(1 << 4, CSR_IRQ_ENABLE);
}
