#ifndef CPHMC_A72_UART_H
#define CPHMC_A72_UART_H

#include <stdint.h>

int uart_putc(char character);
void uart_puts(const char *text);
void uart_put_hex64(uint64_t value);
void uart_put_dec64(uint64_t value);

#endif
