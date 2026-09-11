#include <stdint.h>

#include "platform.h"
#include "timer.h"
#include "uart.h"

static volatile uint64_t g_bss_probe;

static uint64_t read_current_el(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(value));
    return value >> 2;
}

static uint64_t read_mpidr(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(value));
    return value;
}

static uint64_t read_sctlr(uint64_t current_el)
{
    uint64_t value = 0U;

    if (current_el == 3U)
    {
        __asm__ volatile("mrs %0, sctlr_el3" : "=r"(value));
    }
    else if (current_el == 2U)
    {
        __asm__ volatile("mrs %0, sctlr_el2" : "=r"(value));
    }
    else if (current_el == 1U)
    {
        __asm__ volatile("mrs %0, sctlr_el1" : "=r"(value));
    }

    return value;
}

static void print_register(const char *name, uint64_t value)
{
    uart_puts(name);
    uart_put_hex64(value);
    uart_puts("\n");
}

void a72_exception_handler(uint64_t vector_slot,
                           uint64_t exception_level,
                           uint64_t esr,
                           uint64_t elr,
                           uint64_t far)
{
    uart_puts("\nFATAL: A72 exception\n");
    print_register("  vector slot = ", vector_slot);
    print_register("  EL          = ", exception_level);
    print_register("  ESR         = ", esr);
    print_register("  ELR         = ", elr);
    print_register("  FAR         = ", far);

    for (;;)
    {
        __asm__ volatile("wfe");
    }
}

void a72_main(void)
{
    const uint64_t current_el = read_current_el();
    const uint64_t start = timer_counter();
    const uint64_t counter_frequency = timer_frequency();
    uint64_t timer_test_start;
    uint64_t timer_test_elapsed;
    uint64_t heartbeat = 0U;

    uart_puts("\n========================================\n");
    uart_puts(" CPHMC J721E Cortex-A72_0 No-OS\n");
    uart_puts(" build: " __DATE__ " " __TIME__ "\n");
    uart_puts("========================================\n");

    print_register("MPIDR_EL1  = ", read_mpidr());
    print_register("CurrentEL  = ", current_el);
    print_register("SCTLR      = ", read_sctlr(current_el));
    print_register("CNTFRQ_EL0 = ", counter_frequency);
    print_register("UART base  = ", A72_CONSOLE_UART_BASE);

    if (g_bss_probe == 0U)
    {
        uart_puts("BSS test   = PASS\n");
    }
    else
    {
        uart_puts("BSS test   = FAIL\n");
    }
    g_bss_probe = 0xA720A720A720A720ULL;

    if (counter_frequency == 0U)
    {
        uart_puts("Timer test = FAIL (CNTFRQ_EL0 is zero)\n");
        for (;;)
        {
            __asm__ volatile("wfe");
        }
    }

    uart_puts("Timer test = wait 1000 ms ...\n");
    timer_test_start = timer_counter();
    timer_delay_ms(1000U);
    timer_test_elapsed = timer_counter() - timer_test_start;
    if ((timer_test_elapsed + 1000U) >= counter_frequency)
    {
        uart_puts("Timer test = PASS, elapsed ticks=");
        uart_put_dec64(timer_test_elapsed);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Timer test = FAIL, elapsed ticks=");
        uart_put_dec64(timer_test_elapsed);
        uart_puts("\n");
    }
    uart_puts("No scheduler, no RTOS, IRQs masked. Polling heartbeat follows.\n");

    for (;;)
    {
        ++heartbeat;
        uart_puts("heartbeat=");
        uart_put_dec64(heartbeat);
        uart_puts(" counter=");
        uart_put_hex64(timer_counter() - start);
        uart_puts("\n");
        timer_delay_ms(A72_HEARTBEAT_PERIOD_MS);
    }
}
