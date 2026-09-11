/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       uart_ctrl.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      UART control interface for the A72 No-OS image.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef UART_CTRL_H
#define UART_CTRL_H

#include <stdint.h>

int32_t uart_ctrl_init(void);
int uart_ctrl_putc(char character);
int uart_ctrl_getc(void);
uint32_t uart_ctrl_rx_dropped(void);

#endif /* UART_CTRL_H */
