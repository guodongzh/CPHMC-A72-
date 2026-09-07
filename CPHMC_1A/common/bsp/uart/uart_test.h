/**
 *************************************************************************
 * @file      uart.h
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     uart read and write test
 *************************************************************************
 */

#ifndef UART_TEST_H
#define UART_TEST_H

#include "debug_config.h"
#include <string.h>
#include <ti/csl/csl_uart.h>
#include <ti/csl/soc.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/udma/udma.h>
#include <ti/osal/osal.h>
#include <ti/osal/TaskP.h>
#include "udma_apputils.h"


/**
 * @brief hleper macro
 */

#define BAUD_RATE_9600        (9600U)
#define BAUD_RATE_14400       (14400U)
#define BAUD_RATE_19200       (19200U)
#define BAUD_RATE_38400       (38400U)
#define BAUD_RATE_57600       (57600U)
#define BAUD_RATE_115200      (115200U)
#define BAUD_RATE_230400      (230400U)
#define BAUD_RATE_460800      (460800U)
#define BAUD_RATE_921600      (921600U)

#define UART_WORD_LENGTH_5    (UART_FRAME_WORD_LENGTH_5)
#define UART_WORD_LENGTH_6    (UART_FRAME_WORD_LENGTH_6)
#define UART_WORD_LENGTH_7    (UART_FRAME_WORD_LENGTH_7)
#define UART_WORD_LENGTH_8    (UART_FRAME_WORD_LENGTH_8)

#define UART_STOP_BIT_1       (UART_FRAME_NUM_STB_1)
#define UART_STOP_BIT_1_5_2   (UART_FRAME_NUM_STB_1_5_2)

#define UART_NO_PARITY        (UART_PARITY_NONE)
#define UART_PARITY_ODD       (UART_ODD_PARITY)
#define UART_PARTY_EVEN       (UART_EVEN_PARITY)

#define UART_13x_MODE         (UART13x_OPER_MODE)
#define UART_16x_MODE         (UART16x_OPER_MODE)


typedef struct
{
    Udma_DrvHandle drvHandle; /*!< UDMA driver handle */
    Udma_ChHandle chHandle;   /*!< UDMA channel handle */
    void *fqRingMem;          /*!< FQ ring memory pointer*/
    void *cqRingMem;          /*!< CQ ring memory pointer*/
    void *tdRingMem;          /*!< TD ring memory pointer*/
    uint32_t elemCnt;         /*!< Number of elements in ring */
    uint32_t peerChNum;       /*!< Peer channel number */
} UART_DmaParams_t;

typedef struct
{
    uint32_t baseAddr;    /*!< UART instance base address */
    uint32_t baudRate;    /*!< Baud rate */
    uint32_t wordLength;  /*!< Word length */
    uint32_t stopBit;     /*!< Stop bit */
    uint32_t parity;      /*!< Parity */
    uint32_t mode;        /*!< UART mode */
    uint32_t dmaMode;     /*!< DMA mode */
    uint32_t txGra;       /*!< TX FIFO Granularity */
    uint32_t rxGra;       /*!< RX FIFO Granularity */
    uint32_t txTrigLevel; /*!< TX trigger level */
    uint32_t rxTrigLevel; /*!< RX trigger level */
} UART_Params_t;

void UART_Config_Init(UART_Params_t uartParams);
uint32_t UART_Gets(uint32_t baseAddr, char *pRxBuffer, int32_t numBytesToRead);
uint32_t UART_Puts(uint32_t baseAddr,
                   const char *pTxBuffer,
                   int32_t numBytesToWrite);

void uart_init_all();
void uart_test_all();

#endif  // UART_TEST_H
