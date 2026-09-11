/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       timer.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 timer interface.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef A72_BAREMETAL_TIMER_H
#define A72_BAREMETAL_TIMER_H

#include <stdint.h>

void timer_init(void);
uint64_t timer_counter(void);
uint64_t timer_frequency(void);
void timer_delay_ms(uint32_t milliseconds);

#endif
