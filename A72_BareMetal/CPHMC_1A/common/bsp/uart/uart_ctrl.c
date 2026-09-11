/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       uart_ctrl.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      UART control implementation for the A72 No-OS image.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include "bsp/uart/uart_ctrl.h"

#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/uart/src/v1/UART_v1.h>
#include <ti/csl/src/ip/uart/V1/uart.h>

#include "platform.h"

#define CPHMC_WKUP_UART0_IRQ       (929U)
#define CPHMC_UART_INPUT_CLOCK_HZ  (96000000U)

static UART_V1_Object g_uartCtrlObject;
static const UART_HwAttrs g_uartCtrlHwAttrs = {
    A72_CONSOLE_UART_BASE,
    CPHMC_WKUP_UART0_IRQ,
    0U,
    CPHMC_UART_INPUT_CLOCK_HZ,
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0,
    UART_RXTRIGLVL_8,
    UART_TXTRIGLVL_56,
    0U,
    0U,
    1U,
    UART16x_OPER_MODE,
    0,
    0,
    0,
    UART_MDR3_DIR_POL_0,
};

UART_Config UART_config[] = {
    { &UART_FxnTable_v1, &g_uartCtrlObject, &g_uartCtrlHwAttrs },
    { 0, 0, 0 },
};

static int g_uartCtrlInitialized;

int32_t uart_ctrl_init(void)
{
    if (g_uartCtrlInitialized != 0)
    {
        return 0;
    }

    /* WKUP_UART0 is the UART routed to the board connector on this board. */
    UART_stdioInit(0U);
    g_uartCtrlInitialized = 1;
    return 0;
}

int uart_ctrl_putc(char character)
{
    if (g_uartCtrlInitialized == 0)
    {
        return -1;
    }

    UART_putc((uint8_t)character);
    return 0;
}

int uart_ctrl_getc(void)
{
    if (g_uartCtrlInitialized == 0)
    {
        return -1;
    }

    return (int)UART_getc();
}

uint32_t uart_ctrl_rx_dropped(void)
{
    return 0U;
}
