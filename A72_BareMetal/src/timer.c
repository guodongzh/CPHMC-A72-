/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       timer.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 system counter timer support.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include "timer.h"
#include "platform.h"

void timer_init(void)
{
    /*
     * TI performs SoC system-counter enable while still at EL3. Entry does
     * the same on the direct SBL path; a resident ATF owns it otherwise.
     * EL1 therefore only needs a context synchronization before reading it.
     */
    __asm__ volatile("isb" ::: "memory");
}

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
