/**
 *************************************************************************
 * @file      spi.h
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     MCU_MCSPI2 as master send data to MCSPI4
 *************************************************************************
 */

#ifndef SPI_TEST_H
#define SPI_TEST_H

#include "debug_config.h"
#include <string.h>
#include "board/board.h"
#include <ti/csl/csl_mcspi.h>
#include <ti/csl/soc.h>
#include <ti/drv/udma/udma.h>
#include <ti/osal/osal.h>
#include "udma_apputils.h"

typedef enum
{
    TX_RX_MODE = MCSPI_TX_RX_MODE,     /*!< SPI mode MCSPI_TX_RX_MODE */
    RX_ONLY_MODE = MCSPI_RX_ONLY_MODE, /*!< SPI mode MCSPI_RX_ONLY_MODE */
    TX_ONLY_MODE = MCSPI_TX_ONLY_MODE, /*!< SPI mode MCSPI_TX_ONLY_MODE */
} SPI_Mode_t;

typedef enum
{
    SPI_POL0_PHA0_ = MCSPI_CLK_MODE_0, /*!< SPI mode Polarity 0 Phase 0 */
    SPI_POL0_PHA1_ = MCSPI_CLK_MODE_1, /*!< SPI mode Polarity 0 Phase 1 */
    SPI_POL1_PHA0_ = MCSPI_CLK_MODE_2, /*!< SPI mode Polarity 1 Phase 0 */
    SPI_POL1_PHA1_ = MCSPI_CLK_MODE_3, /*!< SPI mode Polarity 1 Phase 1 */
} SPI_FramFormat_t;

typedef struct
{
    uint32_t baseAddr;      /*!< SPI instance base address */
    uint32_t chNum;         /*!< the number of channel */
    uint8_t afl;            /*!< TX trigger level */
    uint8_t ael;            /*!< RX trigger level */
    SPI_Mode_t mode;        /*!< SPI mode */
    bool isMaster;          /*!< master or slave */
    SPI_FramFormat_t frame; /*!< frame format */
    uint8_t wordLen;        /*!< word length (bits)*/
    bool dmaEnable;         /*!< DMA mode */
} SPI_Params_t;

typedef struct
{
    Udma_DrvHandle drvHandle; /*!< UDMA driver handle */
    Udma_ChHandle chHandle;   /*!< UDMA channel handle */
    void *fqRingMem;          /*!< FQ ring memory pointer*/
    void *cqRingMem;          /*!< CQ ring memory pointer*/
    void *tdRingMem;          /*!< TD ring memory pointer*/
    uint32_t elemCnt;         /*!< Number of elements in ring */
    uint32_t peerChNum;       /*!< Peer channel number */
} SPI_DmaParams_t;

void spi_init(SPI_Params_t param);
int32_t spi_tx_udma_config(SPI_DmaParams_t *params);
int32_t spi_rx_udma_config(SPI_DmaParams_t *params);
int32_t
spi_tx_pdma_config(Udma_ChHandle chHandle, uint32_t fifoLevel, size_t size, uint32_t wordLen);
int32_t
spi_rx_pdma_config(Udma_ChHandle chHandle, uint32_t fifioLevel, size_t size, uint32_t elemSize);
void spi_csl_test();
void spi_udma_test();

#endif // SPI_TEST_H
