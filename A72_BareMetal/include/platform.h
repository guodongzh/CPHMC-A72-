/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       platform.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 platform definitions.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef A72_BAREMETAL_PLATFORM_H
#define A72_BAREMETAL_PLATFORM_H

#include <stdint.h>

#define J721E_MAIN_UART0_BASE       (0x02800000UL)
#define J721E_WKUP_UART0_BASE       (0x42300000UL)

/* J721E Global Timebase Counter (GTC), fixed at 200 MHz. */
#define J721E_GTC_CFG1_BASE         (0x00A90000UL)
#define J721E_GTC_CNTCR_OFFSET      (0x00UL)
#define J721E_GTC_CNTFID0_OFFSET    (0x20UL)
#define J721E_GTC_CNTCR_ENABLE      (1UL << 0)
#define J721E_GTC_FREQUENCY_HZ      (200000000UL)

#ifndef A72_CONSOLE_UART_BASE
#define A72_CONSOLE_UART_BASE       J721E_WKUP_UART0_BASE
#endif

#define UART_THR_OFFSET             (0x00UL)
#define UART_LSR_OFFSET             (0x14UL)
#define UART_LSR_THRE               (1UL << 5)

#define A72_HEARTBEAT_PERIOD_MS     (1000UL)
#define A72_UART_POLL_LIMIT         (10000000UL)

static inline uint32_t mmio_read32(uintptr_t address)
{
    return *(volatile uint32_t *)address;
}

static inline void mmio_write32(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}

#endif
