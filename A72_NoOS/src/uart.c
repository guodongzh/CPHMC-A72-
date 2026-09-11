#include "platform.h"
#include "uart.h"

int uart_putc(char character)
{
    uint32_t remaining = A72_UART_POLL_LIMIT;

    while ((mmio_read32(A72_CONSOLE_UART_BASE + UART_LSR_OFFSET) & UART_LSR_THRE) == 0U)
    {
        if (--remaining == 0U)
        {
            return -1;
        }
    }

    mmio_write32(A72_CONSOLE_UART_BASE + UART_THR_OFFSET, (uint32_t)(uint8_t)character);
    return 0;
}

void uart_puts(const char *text)
{
    while (*text != '\0')
    {
        if (*text == '\n')
        {
            (void)uart_putc('\r');
        }
        (void)uart_putc(*text++);
    }
}

void uart_put_hex64(uint64_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    int shift;

    uart_puts("0x");
    for (shift = 60; shift >= 0; shift -= 4)
    {
        (void)uart_putc(digits[(value >> (uint32_t)shift) & 0xFU]);
    }
}

void uart_put_dec64(uint64_t value)
{
    char buffer[21];
    uint32_t used = 0U;

    if (value == 0U)
    {
        (void)uart_putc('0');
        return;
    }

    while (value != 0U)
    {
        buffer[used++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (used != 0U)
    {
        (void)uart_putc(buffer[--used]);
    }
}
