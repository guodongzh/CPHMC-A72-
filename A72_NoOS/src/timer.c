#include "timer.h"

uint64_t timer_counter(void)
{
    uint64_t value;
    __asm__ volatile("isb\n\tmrs %0, cntpct_el0" : "=r"(value));
    return value;
}

uint64_t timer_frequency(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

void timer_delay_ms(uint32_t milliseconds)
{
    const uint64_t start = timer_counter();
    const uint64_t ticks = (timer_frequency() / 1000U) * milliseconds;

    while ((timer_counter() - start) < ticks)
    {
        __asm__ volatile("yield");
    }
}
