#ifndef CPHMC_A72_PLATFORM_H
#define CPHMC_A72_PLATFORM_H

#include <stdint.h>

/* J721E UART register blocks use 32-bit register spacing. */
#define J721E_MAIN_UART0_BASE       (0x02800000UL)
#define J721E_WKUP_UART0_BASE       (0x42300000UL)

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
