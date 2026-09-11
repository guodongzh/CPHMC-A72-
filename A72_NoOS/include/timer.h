#ifndef CPHMC_A72_TIMER_H
#define CPHMC_A72_TIMER_H

#include <stdint.h>

uint64_t timer_counter(void);
uint64_t timer_frequency(void);
void timer_delay_ms(uint32_t milliseconds);

#endif
