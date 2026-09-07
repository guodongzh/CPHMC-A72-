/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       RS485.c
 *@author     xuesen
 *@date       2024.12.24
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.12.24  1.0       xuesen     example
 ******************************************************************************/

#include "uart_test.h"
#include "rs485.h"
#include "gpio_ctrl.h"

/* ring setting */
#define RING_ELEMENT_CNT    (10U)
#define RING_ELEMENT_SIZE   (8U) /* 8 bytes */

/* APP */
#define BUF_CNT             RING_ELEMENT_CNT
#define BUF_SIZE            (128u)

/**
 * @brief hleper macro
 */

#define BAUD_RATE_9600      (9600U)
#define BAUD_RATE_14400     (14400U)
#define BAUD_RATE_19200     (19200U)
#define BAUD_RATE_38400     (38400U)
#define BAUD_RATE_57600     (57600U)
#define BAUD_RATE_115200    (115200U)
#define BAUD_RATE_230400    (230400U)
#define BAUD_RATE_460800    (460800U)
#define BAUD_RATE_921600    (921600U)

#define UART_WORD_LENGTH_5  (UART_FRAME_WORD_LENGTH_5)
#define UART_WORD_LENGTH_6  (UART_FRAME_WORD_LENGTH_6)
#define UART_WORD_LENGTH_7  (UART_FRAME_WORD_LENGTH_7)
#define UART_WORD_LENGTH_8  (UART_FRAME_WORD_LENGTH_8)

#define UART_STOP_BIT_1     (UART_FRAME_NUM_STB_1)
#define UART_STOP_BIT_1_5_2 (UART_FRAME_NUM_STB_1_5_2)

#define UART_NO_PARITY      (UART_PARITY_NONE)
#define UART_PARITY_ODD     (UART_ODD_PARITY)
#define UART_PARTY_EVEN     (UART_EVEN_PARITY)

#define UART_13x_MODE       (UART13x_OPER_MODE)
#define UART_16x_MODE       (UART16x_OPER_MODE)

/**
 * @brief uart parameter
 */
#define UART_TEST_BASE      CSL_UART2_BASE
#define BAUD_RATE           BAUD_RATE_115200
#define WORD_LENGTH         UART_WORD_LENGTH_8
#define STOP_BIT            UART_STOP_BIT_1
#define PARITY              UART_NO_PARITY
#define UART_MODE           UART_16x_MODE

#define UART0_TEST_BASE     CSL_UART0_BASE
#define BAUD_RATE           BAUD_RATE_115200
#define WORD_LENGTH         UART_WORD_LENGTH_8
#define STOP_BIT            UART_STOP_BIT_1
#define PARITY              UART_NO_PARITY
#define UART_MODE           UART_16x_MODE

#define UART8_TEST_BASE     CSL_UART8_BASE
#define BAUD_RATE           BAUD_RATE_115200
#define WORD_LENGTH         UART_WORD_LENGTH_8
#define STOP_BIT            UART_STOP_BIT_1
#define PARITY              UART_NO_PARITY
#define UART_MODE           UART_16x_MODE

#define UART9_TEST_BASE     CSL_UART9_BASE
#define BAUD_RATE           BAUD_RATE_115200
#define WORD_LENGTH         UART_WORD_LENGTH_8
#define STOP_BIT            UART_STOP_BIT_1
#define PARITY              UART_NO_PARITY
#define UART_MODE           UART_16x_MODE

void rs485_init()
{
    /* config uart */
    UART_Params_t uartParams;
    uartParams.baseAddr = UART8_TEST_BASE;
    uartParams.baudRate = BAUD_RATE;
    uartParams.wordLength = WORD_LENGTH;
    uartParams.stopBit = STOP_BIT;
    uartParams.parity = PARITY;
    uartParams.mode = UART_MODE;
    uartParams.dmaMode = UART_DMA_MODE_0_ENABLE;
    uartParams.txGra = UART_TRIG_LVL_GRANULARITY_4;
    uartParams.rxGra = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.txTrigLevel = 56; /* FIFO transmit space setting */
    uartParams.rxTrigLevel = 1;
    UART_Config_Init(uartParams);

    uartParams.baseAddr = UART9_TEST_BASE;
    uartParams.baudRate = BAUD_RATE;
    uartParams.wordLength = WORD_LENGTH;
    uartParams.stopBit = STOP_BIT;
    uartParams.parity = PARITY;
    uartParams.mode = UART_MODE;
    uartParams.dmaMode = UART_DMA_MODE_0_ENABLE;
    uartParams.txGra = UART_TRIG_LVL_GRANULARITY_4;
    uartParams.rxGra = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.txTrigLevel = 56; /* FIFO transmit space setting */
    uartParams.rxTrigLevel = 1;
    UART_Config_Init(uartParams);
}


static int change_rs485_dir = 0;
int rs485_uart8_tx = 0;
rs485_diag_t rs485_diag;
void rs485_change_dir(int dir)
{
    if (dir != 0 && change_rs485_dir == 0)
    {
        if (rs485_uart8_tx == 0)
            rs485_uart8_tx = 1;
        else
            rs485_uart8_tx = 0;
        if (rs485_uart8_tx)
        {
            Debug_logInfo("\nRS485 UART8 TXEN\n");
            gpio_enable_rs485_uart8_txen(1);
            gpio_enable_rs485_uart9_txen(0);
        }
        else
        {
            Debug_logInfo("\nRS485 UART9 TXEN\n");
            gpio_enable_rs485_uart8_txen(0);
            gpio_enable_rs485_uart9_txen(1);
        }
        rs485_diag.recv_oks = 0;
        rs485_diag.recv_err = 0;
    }
    change_rs485_dir = dir;
}


void rs485_loop_back()
{
    static uint8_t send_buffer = 0;
    static uint8_t recv_buffer = 0;
    uint32_t send_baseAddr;
    uint32_t recv_baseAddr;
    if (rs485_uart8_tx)
    {
        send_baseAddr = CSL_UART8_BASE;
        recv_baseAddr = CSL_UART9_BASE;
    }
    else
    {
        send_baseAddr = CSL_UART9_BASE;
        recv_baseAddr = CSL_UART8_BASE;
    }
    UARTCharPut(send_baseAddr, send_buffer);
    Osal_delay(1);
    if (UARTCharGetTimeout2(recv_baseAddr, 100, &recv_buffer))
    {
        if (recv_buffer != send_buffer)
        {
            Debug_logError("rs485 loop back error\n");
            rs485_diag.recv_err++;
        }
        else
        {
//            Debug_logOk("rs485 loop back ok\n");
            rs485_diag.recv_oks++;
        }
    }
    else
    {
        Debug_logError("rs485 loop back timeout\n");
        rs485_diag.recv_err++;
    }
    send_buffer++;
}


