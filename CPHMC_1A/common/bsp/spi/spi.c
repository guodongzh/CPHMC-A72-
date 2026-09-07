/**
 *************************************************************************
 * @file      spi.c
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     MCU_MCSPI2 as master send data to MCSPI4
 *************************************************************************
 */

#include <udma_mem_copy.h>
#include "spi.h"

/* UDMA Ring Memory Pointer */
/* ring setting */
#define RING_ELEMENT_CNT  (10U)
#define RING_ELEMENT_SIZE (8U) /* 8 bytes */

uint8_t gSpiTxFqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiTxCqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiTxTdRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiTxPdMem[RING_ELEMENT_CNT][UDMA_TRPD_SIZE_ALIGN]__attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

uint8_t gSpiRxFqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiRxCqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiTdRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiRxPdMem[RING_ELEMENT_CNT][UDMA_TRPD_SIZE_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

/**
 * UDMA driver objects
 */
struct Udma_ChObj gSpiUdmaTxChObj;
struct Udma_ChObj gSpiUdmaRxChObj;
struct Udma_EventObj gSpiUdmaTxCqEventObj;
struct Udma_EventObj gSpiUdmaRxCqEventObj;

/*APP*/
#define BUF_CNT                   RING_ELEMENT_CNT
#define BUF_SIZE                  (128u)
#define FIFO_TRIGGER_LEVEL        (32u)
#define Master_MCSPI_BASE_ADDRESS CSL_MCU_MCSPI2_CFG_BASE
#define Slave_MCSPI_BASE_ADDRESS  CSL_MCSPI4_CFG_BASE
uint8_t gRxBuffer[BUF_CNT][BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gTxBuffer[BUF_CNT][BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gTxSlvBuffer[BUF_CNT][BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gRxSlvBuffer[BUF_CNT][BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));



/**
 * @brief This function will call the necessary McSPI APIs which will
 *    configure the McSPI controller.
 * @param param SPI parameters
 */
void spi_init(SPI_Params_t param)
{
    uint32_t status = 1U;          /* FALSE */
    uint32_t spiInClk = 50000000U;  /* 50MHz */
    uint32_t spiOutClk = 12500000U; /* 12.5MHz */

    /* Reset the McSPI instance.*/
    McSPIReset(param.baseAddr);

    /* CLOCKACTIVITY bit - OCP and Functional clocks are maintained           */
    /* SIDLEMODE     bit - Ignore the idle request and configure in normal mode
     */
    /* AUTOIDLE      bit - Disable (OCP clock is running free, no gating)     */
    MCSPISysConfigSetup(param.baseAddr,
                        MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO,
                        MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);

    /* Enable chip select pin.*/
    McSPICSEnable(param.baseAddr);

    /* Enable master/slave mode of operation.*/
    if (param.isMaster)
    {
        McSPIMasterModeEnable(param.baseAddr);
        status = McSPIMasterModeConfig(param.baseAddr,
                                       MCSPI_SINGLE_CH,
                                       param.mode,
                                       MCSPI_DATA_LINE_COMM_MODE_6,
                                       param.chNum);
        if (0U == status)
        {
            SPI_log("Communication not supported by SPIDAT[1:0]\n");
        }
    }
    else
    {
        McSPISlaveModeEnable(param.baseAddr);
        status = MCSPIPinDirSet(param.baseAddr,
                                param.mode,
                                MCSPI_DATA_LINE_COMM_MODE_1,
                                param.chNum);
        if (1U == status)
        {
            SPI_log("Communication not supported by SPIDAT[1:0]\n");
        }
    }

    /* Configure the McSPI bus clock depending on frame format. */
    McSPIClkConfig(param.baseAddr, spiInClk, spiOutClk, param.chNum, param.frame);

    /* Configure the word length.*/
    McSPIWordLengthSet(param.baseAddr, MCSPI_WORD_LENGTH(param.wordLen), param.chNum);

    /* Set polarity of SPIEN to low.*/
    McSPICSPolarityConfig(param.baseAddr,
                          (MCSPI_CH0CONF_EPOL_ACTIVELOW << MCSPI_CH0CONF_EPOL_SHIFT),
                          param.chNum);

    /* Enable the transmitter/receiver FIFO and dma request of McSPI peripheral.*/
    switch (param.mode)
    {
    case TX_RX_MODE:
        McSPITxFIFOConfig(param.baseAddr, MCSPI_TX_FIFO_ENABLE, param.chNum);
        McSPIRxFIFOConfig(param.baseAddr, MCSPI_RX_FIFO_ENABLE, param.chNum);
        if (param.dmaEnable)
        {
            McSPIDMAEnable(param.baseAddr, MCSPI_DMA_TX_EVENT, param.chNum);
            McSPIDMAEnable(param.baseAddr, MCSPI_DMA_RX_EVENT, param.chNum);
        }
        break;
    case TX_ONLY_MODE:
        McSPITxFIFOConfig(param.baseAddr, MCSPI_TX_FIFO_ENABLE, param.chNum);
        if (param.dmaEnable)
        {
            McSPIDMAEnable(param.baseAddr, MCSPI_DMA_TX_EVENT, param.chNum);
        }
        break;
    case RX_ONLY_MODE:
        McSPIRxFIFOConfig(param.baseAddr, MCSPI_RX_FIFO_ENABLE, param.chNum);
        if (param.dmaEnable)
        {
            McSPIDMAEnable(param.baseAddr, MCSPI_DMA_RX_EVENT, param.chNum);
        }
        break;
    }

    /* Set FIFO levels.*/
    McSPIFIFOTrigLvlSet(param.baseAddr, param.afl, param.ael, param.mode);

    /* Enable the McSPI channel for communication.*/
    McSPIChannelEnable(param.baseAddr, param.chNum);
}

/**
 *  @brief Initialize data buffer
 */
void Init_Buff(void)
{
    uint32_t index, i;

    for (i = 0; i < BUF_CNT; i++)
    {
        for (index = 0; index < BUF_SIZE; index++)
        {
            /* Initialize the gTxBuffer McSPI1 with a known pattern of data */
            gTxBuffer[i][index] = (uint8_t)index + 1U;
            /* Initialize the gRxBuffer McSPI1 with 0 */
            gRxBuffer[i][index] = (uint8_t)0;
        }
        /* Writeback source buffer */
        CacheP_wbInv((void *)&gTxBuffer[i][0], (int32_t)BUF_SIZE);
        CacheP_wbInv((void *)&gRxBuffer[i][0], (int32_t)BUF_SIZE);

        for (index = 0; index < BUF_SIZE; index++)
        {
            /* Initialize the gTxBuffer McSPI1 with a known pattern of data */
            gTxSlvBuffer[i][index] = (uint8_t)index + 1U;
            /* Initialize the gRxBuffer McSPI1 with 0 */
            gRxSlvBuffer[i][index] = 0;
        }
        /* Writeback source buffer */
        CacheP_wbInv((void *)&gTxSlvBuffer[i][0], (int32_t)BUF_SIZE);
        CacheP_wbInv((void *)&gRxSlvBuffer[i][0], (int32_t)BUF_SIZE);
    }
}

bool Verify_Data(const uint8_t *data1_ptr, const uint8_t *data2_ptr)
{
    uint32_t index, i;
    bool gEqual = true;
    uint8_t data1[BUF_CNT][BUF_SIZE];
    uint8_t data2[BUF_CNT][BUF_SIZE];

    if (data1_ptr == NULL || data2_ptr == NULL)
    {
        SPI_log("Verify_Data: Invalid input parameters\r\n");
        return false;
    }
    memcpy(data1, data1_ptr, sizeof(data1));
    memcpy(data2, data2_ptr, sizeof(data2));

    for (i = 0; i < BUF_CNT; ++i)
    {
        for (index = 0; index < BUF_SIZE; index++)
        {
            if ((data1[i][index] != data2[i][index]))
            {
                SPI_log("Data Mismatch found at index : %u\r\n", index);
                gEqual = false;
                break;
            }
        }
    }
    return gEqual;
}

void SPI_Polled_Transfer(uint16_t length,
                         uint8_t *p_tx,
                         uint8_t *p_rx,
                         uint8_t *p_txSlv,
                         uint8_t *p_rxSlv)
{
    uint32_t channelStatus;
    volatile uint32_t timeout1;
    uint8_t i;

    /*send data one by one*/
    while (0 != length)
    {
        timeout1 = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(Slave_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
        while (0 == (channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = McSPIChannelStatusGet(Slave_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
            --timeout1;
            if (0 == timeout1)
            {
                SPI_log("\nMAIN McSPI4 TX Timed out!!");
                while (1)
                    ;
            }
        }
        /*MCSPI4 as slave send data to MCU_MCSPI2 */
        McSPITransmitData(Slave_MCSPI_BASE_ADDRESS, (uint32_t)(*p_txSlv++), MCSPI_CHANNEL_0);

        timeout1 = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
        while (0 == (channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
            --timeout1;
            if (0 == timeout1)
            {
                SPI_log("\nMCU McSPI2 TX Timed out!!");
                while (1)
                    ;
            }
        }

        /*MCU_MCSPI2 as master send data to MCSPI4*/
        McSPITransmitData(Master_MCSPI_BASE_ADDRESS, (uint32_t)(*p_tx++), MCSPI_CHANNEL_0);

        timeout1 = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
        while (0 == (channelStatus & MCSPI_CH0STAT_RXS_MASK))
        {
            channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
            --timeout1;
            if (0 == timeout1)
            {
                SPI_log("\nMCU McSPI2 RX Timed out!!");
                while (1)
                    ;
            }
        }
        *p_rx++ = (uint8_t)McSPIReceiveData(Master_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);

        timeout1 = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(Slave_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
        while (0 == (channelStatus & MCSPI_CH0STAT_RXS_MASK))
        {
            channelStatus = McSPIChannelStatusGet(Slave_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
            --timeout1;
            if (0 == timeout1)
            {
                SPI_log("\nMAIN McSPI4 RX Timed out!!");
                while (1)
                    ;
            }
        }
        *p_rxSlv++ = (uint8_t)McSPIReceiveData(Slave_MCSPI_BASE_ADDRESS, MCSPI_CHANNEL_0);
        length--;
    }
}

/**
 * @brief SPI CSL task
 */
void spi_csl_test()
{
    SPI_Params_t masterParam;
    SPI_Params_t slaveParam;

    SPI_log("\nspi test started...\n");

    /* config master SPI */
    masterParam.baseAddr = Master_MCSPI_BASE_ADDRESS;
    masterParam.chNum = MCSPI_CHANNEL_0;
    masterParam.afl = FIFO_TRIGGER_LEVEL;
    masterParam.ael = FIFO_TRIGGER_LEVEL;
    masterParam.mode = TX_RX_MODE;
    masterParam.isMaster = true;
    masterParam.frame = SPI_POL0_PHA0_;
    masterParam.wordLen = 8;
    masterParam.dmaEnable = false;
    spi_init(masterParam);

    /* config slave SPI */
    slaveParam.baseAddr = Slave_MCSPI_BASE_ADDRESS;
    slaveParam.chNum = MCSPI_CHANNEL_0;
    slaveParam.afl = FIFO_TRIGGER_LEVEL;
    slaveParam.ael = FIFO_TRIGGER_LEVEL;
    slaveParam.mode = TX_RX_MODE;
    slaveParam.isMaster = false;
    slaveParam.frame = SPI_POL0_PHA0_;
    slaveParam.wordLen = 8;
    slaveParam.dmaEnable = false;
    spi_init(slaveParam);

    /* Transfer */
    Init_Buff();
    for (int i = 0; i < BUF_CNT; ++i)
    {
        /* SPIEN line is forced to low state.*/
        McSPICSAssert(masterParam.baseAddr, masterParam.chNum);
        McSPICSAssert(slaveParam.baseAddr, slaveParam.chNum);

        SPI_Polled_Transfer(BUF_SIZE,
                            &gTxBuffer[i][0],
                            &gRxBuffer[i][0],
                            &gTxSlvBuffer[i][0],
                            &gRxSlvBuffer[i][0]);

        /* Force SPIEN line to the inactive state.*/
        McSPICSDeAssert(masterParam.baseAddr, masterParam.chNum);
        McSPICSDeAssert(slaveParam.baseAddr, slaveParam.chNum);
    }

    /*verify*/
    if (Verify_Data(&gTxBuffer[0][0], &gRxSlvBuffer[0][0]) == true &&
        Verify_Data(&gTxSlvBuffer[0][0], &gRxBuffer[0][0]) == true)
    {
        SPI_log("McSPI Polled Mode Transfer Test Passed\n");
    }
    else
    {
        SPI_log("McSPI Polled Mode Transfer Test Fail\n");
    }

    SPI_log("spi_csl_test Passed\n");
}

/**
 * @brief Configuring UDMA for uart transmit
 * @return UDMA_SOK if success, else error code
 */
int32_t spi_tx_udma_config(SPI_DmaParams_t *params)
{
    uint32_t chType;
    int32_t retVal;
    Udma_ChTxPrms txPrms;
    Udma_ChPrms chPrms;

    /* Init TX channel parameters */
    if (params->fqRingMem == NULL || params->cqRingMem == NULL || params->tdRingMem == NULL ||
        params == NULL)
    {
        retVal = UDMA_EALLOC;
        return retVal;
    }
    chType = UDMA_CH_TYPE_PDMA_TX;
    UdmaChPrms_init(&chPrms, chType);
    chPrms.peerChNum = params->peerChNum;
    chPrms.fqRingPrms.ringMem = params->fqRingMem;
    chPrms.cqRingPrms.ringMem = params->cqRingMem;
    chPrms.tdCqRingPrms.ringMem = params->tdRingMem;
    chPrms.fqRingPrms.elemCnt = params->elemCnt;
    chPrms.cqRingPrms.elemCnt = params->elemCnt;
    chPrms.tdCqRingPrms.elemCnt = params->elemCnt;
    retVal = Udma_chOpen(params->drvHandle, params->chHandle, chType, &chPrms);
    if (retVal != UDMA_SOK)
    {
        SPI_log("UART_TX_UDMA_Config: UDMA TX channel open failed!!\n");
        return retVal;
    }
    UdmaChTxPrms_init(&txPrms, chType);
    retVal = Udma_chConfigTx(params->chHandle, &txPrms);
    if (retVal != UDMA_SOK)
    {
        SPI_log("spi_tx_udma_config: UDMA TX channel config failed!!\n");
        return retVal;
    }
    return retVal;
}

/**
 * @brief Configuring UDMA for uart receive
 * @return UDMA_SOK if success, else error code
 */
int32_t spi_rx_udma_config(SPI_DmaParams_t *params)
{
    uint32_t chType;
    int32_t retVal;
    Udma_ChRxPrms rxPrms;
    Udma_ChPrms chPrms;

    /* Init TX channel parameters */
    if (params->fqRingMem == NULL || params->cqRingMem == NULL || params->tdRingMem == NULL)
    {
        retVal = UDMA_EALLOC;
        return retVal;
    }
    chType = UDMA_CH_TYPE_PDMA_RX;
    UdmaChPrms_init(&chPrms, chType);
    chPrms.peerChNum = params->peerChNum;
    chPrms.fqRingPrms.ringMem = params->fqRingMem;
    chPrms.cqRingPrms.ringMem = params->cqRingMem;
    chPrms.tdCqRingPrms.ringMem = params->tdRingMem;
    chPrms.fqRingPrms.elemCnt = params->elemCnt;
    chPrms.cqRingPrms.elemCnt = params->elemCnt;
    chPrms.tdCqRingPrms.elemCnt = params->elemCnt;
    retVal = Udma_chOpen(params->drvHandle, params->chHandle, chType, &chPrms);
    if (retVal != UDMA_SOK)
    {
        SPI_log("spi_rx_udma_config: UDMA TX channel open failed!!\n");
        return retVal;
    }
    UdmaChRxPrms_init(&rxPrms, chType);
    retVal = Udma_chConfigRx(params->chHandle, &rxPrms);
    if (retVal != UDMA_SOK)
    {
        SPI_log("spi_rx_udma_config: UDMA TX channel config failed!!\n");
        return retVal;
    }
    return retVal;
}

/**
 * @brief SPI transmit PDMA configuration
 * @param chHandle UDMA channel handle
 * @param fifoLevel SPI FIFO level
 * @param size BUffer size
 * @param wordLen word length,must be 8/16/24/32
 * @return UDMA_SOK if success, else error code
 */
int32_t
spi_tx_pdma_config(Udma_ChHandle chHandle, uint32_t fifoLevel, size_t size, uint32_t wordLen)
{
    int32_t retVal;
    Udma_ChPdmaPrms pdmaPrms;

    UdmaChPdmaPrms_init(&pdmaPrms);
    pdmaPrms.elemSize = (uint32_t)(wordLen / 8 - 1);
    if ((uint32_t)size > fifoLevel)
    {
        pdmaPrms.elemCnt = fifoLevel;
    }
    else
    {
        pdmaPrms.elemCnt = (uint32_t)size;
    }
    pdmaPrms.fifoCnt = 0U; /* Don't care for write */
    retVal = Udma_chConfigPdma(chHandle, &pdmaPrms);

    if (UDMA_SOK == retVal)
    {
        retVal = Udma_chEnable(chHandle);
        if (UDMA_SOK != retVal)
        {
            SPI_log("UDMA App: UDMA TX channel enable failed!!\n");
        }
    }

    return retVal;
}

/**
 * @brief SPI receive PDMA configuration
 * @param chHandle UDMA channel handle
 * @param fifioLevel SPI FIFO level
 * @param size BUffer size
 * @param wordLen word length,must be 8/16/24/32
 * @return UDMA_SOK if success, else error code
 */
int32_t
spi_rx_pdma_config(Udma_ChHandle chHandle, uint32_t fifioLevel, size_t size, uint32_t elemSize)
{
    int32_t retVal;
    uint32_t rxSize;
    Udma_ChPdmaPrms pdmaPrms;

    if ((uint32_t)size <= fifioLevel)
    {
        rxSize = (uint32_t)size;
    }
    else
    {
        rxSize = ((uint32_t)size / fifioLevel) * fifioLevel;
    }

    UdmaChPdmaPrms_init(&pdmaPrms);
    pdmaPrms.elemSize = (uint32_t)(elemSize / 8 - 1);
    if (rxSize > fifioLevel)
    {
        pdmaPrms.elemCnt = fifioLevel;
        pdmaPrms.fifoCnt = rxSize / fifioLevel;
    }
    else
    {
        pdmaPrms.elemCnt = rxSize;
        pdmaPrms.fifoCnt = 1U;
    }

    retVal = Udma_chConfigPdma(chHandle, &pdmaPrms);

    if (UDMA_SOK == retVal)
    {
        retVal = Udma_chEnable(chHandle);
    }

    return retVal;
}

/**
 * @brief SPI UDMA Task
 */
void spi_udma_test()
{
    int32_t retVal = UDMA_SOK;
    uint32_t i, j;
    uint32_t buffSize = BUF_SIZE;
    CSL_UdmapCppi5HMPD *pHpdMem;
    uint32_t occ;
    Udma_ChStats chStats;
    uint64_t phyDescAddr;
    void *appBufPtr;
    SPI_Params_t masterParam;
    SPI_Params_t slaveParam;
    SPI_DmaParams_t dmaParams;

    SPI_log("%s: Starting!!\r\n", __func__);

    /* config tx udma */
    dmaParams.drvHandle = &gUdmaDrvObj;
    dmaParams.chHandle = &gSpiUdmaTxChObj;
    dmaParams.fqRingMem = gSpiTxFqRingMem;
    dmaParams.cqRingMem = gSpiTxCqRingMem;
    dmaParams.tdRingMem = gSpiTxTdRingMem;
    dmaParams.elemCnt = RING_ELEMENT_CNT;
    dmaParams.peerChNum = CSL_PDMA_CH_MCU_MCSPI2_CH0_TX;
    retVal = spi_tx_udma_config(&dmaParams);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA config failed\r\n", __func__);
        goto TESTEND;
    }

    /* config rx udma */
    dmaParams.drvHandle = &gUdmaDrvObj;
    dmaParams.chHandle = &gSpiUdmaRxChObj;
    dmaParams.fqRingMem = gSpiRxFqRingMem;
    dmaParams.cqRingMem = gSpiRxCqRingMem;
    dmaParams.tdRingMem = gSpiTdRingMem;
    dmaParams.elemCnt = RING_ELEMENT_CNT;
    dmaParams.peerChNum = CSL_PDMA_CH_MAIN_MCSPI4_CH0_RX;
    retVal = spi_rx_udma_config(&dmaParams);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA config failed\r\n", __func__);
        goto TESTEND;
    }

    /* config spi */
    /* config master SPI */
    masterParam.baseAddr = Master_MCSPI_BASE_ADDRESS;
    masterParam.chNum = MCSPI_CHANNEL_0;
    masterParam.afl = FIFO_TRIGGER_LEVEL;
    masterParam.ael = FIFO_TRIGGER_LEVEL;
    masterParam.mode = TX_ONLY_MODE;
    masterParam.isMaster = true;
    masterParam.frame = SPI_POL0_PHA0_;
    masterParam.wordLen = 8;
    masterParam.dmaEnable = true;
    spi_init(masterParam);

    /* config slave SPI */
    slaveParam.baseAddr = Slave_MCSPI_BASE_ADDRESS;
    slaveParam.chNum = MCSPI_CHANNEL_0;
    slaveParam.afl = FIFO_TRIGGER_LEVEL;
    slaveParam.ael = FIFO_TRIGGER_LEVEL;
    slaveParam.mode = RX_ONLY_MODE;
    slaveParam.isMaster = false;
    slaveParam.frame = SPI_POL0_PHA0_;
    slaveParam.wordLen = 8;
    slaveParam.dmaEnable = true;
    spi_init(slaveParam);

    /* config pdma */
    retVal = spi_tx_pdma_config(&gSpiUdmaTxChObj, masterParam.afl, buffSize, masterParam.wordLen);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA TX channel config failed\r\n", __func__);
        goto TESTEND;
    }
    retVal = spi_rx_pdma_config(&gSpiUdmaRxChObj, slaveParam.ael, buffSize, slaveParam.wordLen);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA RX channel config failed\r\n", __func__);
        goto TESTEND;
    }

    Init_Buff();

    /* build host packet descriptor */
    for (i = 0; i < RING_ELEMENT_CNT; ++i)
    {
        pHpdMem = (CSL_UdmapCppi5HMPD*)&gSpiTxPdMem[i][0];
        if (NULL == pHpdMem)
        {
            SPI_log("%s:Failed to allocate memory for TX HPD\n", __func__);
            goto TESTEND;
        }
        UDMA_Hpd_Init(&gSpiUdmaTxChObj,
                      pHpdMem, Udma_appVirtToPhyFxn(&gTxBuffer[i][0]),
                      buffSize);
        retVal = Udma_ringQueueRaw(Udma_chGetCqRingHandle(&gSpiUdmaTxChObj),
                                   Udma_appVirtToPhyFxn(pHpdMem));
        if (retVal != UDMA_SOK)
        {
            SPI_log("%s:Failed to submit TX CQ code: %d \n", __func__, retVal);
            goto TESTEND;
        }

        pHpdMem = (CSL_UdmapCppi5HMPD*)&gSpiRxPdMem[i][0];
        if (NULL == pHpdMem)
        {
            SPI_log("%s:Failed to allocate memory for TX HPD\n", __func__);
            goto TESTEND;
        }
        UDMA_Hpd_Init(&gSpiUdmaRxChObj,
                      pHpdMem,
                      Udma_appVirtToPhyFxn(&gRxSlvBuffer[i][0]),
                      buffSize);
        retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gSpiUdmaRxChObj),
                                   Udma_appVirtToPhyFxn(pHpdMem));
        if (retVal != UDMA_SOK)
        {
            SPI_log("%s:Failed to submit RX FQ\n", __func__);
            goto TESTEND;
        }
    }

    SPI_log("\n");
    occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gSpiUdmaTxChObj));
    SPI_log("%s:tx cq occ %d \n", __func__, occ);
    occ = Udma_ringGetForwardRingOcc(Udma_chGetFqRingHandle(&gSpiUdmaRxChObj));
    SPI_log("%s:rx fq occ %d \n", __func__, occ);
    SPI_log("\n");

    /* pop dp from completion queue, fill data, push to forward queue*/
    McSPICSAssert(masterParam.baseAddr, masterParam.chNum);
    McSPICSAssert(slaveParam.baseAddr, slaveParam.chNum);
    Udma_chPause(&gSpiUdmaTxChObj);
    for (i = 0; i < RING_ELEMENT_CNT; ++i)
    {
        retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gSpiUdmaTxChObj), &phyDescAddr);
        if (UDMA_SOK != retVal)
        {
            SPI_log("%s:Udma_ringDequeueRaw fail %d\r\n", __func__, retVal);
            SPI_log("\r\n");
            goto TESTEND;
        }
        pHpdMem = (CSL_UdmapCppi5HMPD *)Udma_appPhyToVirtFxn(phyDescAddr);

        CacheP_wbInv((const void *)pHpdMem, (int32_t)(sizeof(CSL_UdmapCppi5HMPD)));

        /*user write buffer*/
        appBufPtr = Udma_appPhyToVirtFxn(pHpdMem->orgBufPtr);
        memset(appBufPtr, 0, buffSize);
        ((char *)appBufPtr)[0] = '\n';
        ((char *)appBufPtr)[1] = 'B';
        ((char *)appBufPtr)[2] = 'E';
        ((char *)appBufPtr)[3] = 'G';
        ((char *)appBufPtr)[4] = 'I';
        ((char *)appBufPtr)[5] = 'N';
        ((char *)appBufPtr)[6] = '\n';
        for (j = 7; j < (buffSize - 5); j++)
        {
            ((char *)appBufPtr)[j] = '0' + i;
        }
        ((char *)appBufPtr)[buffSize - 5] = '\n';
        ((char *)appBufPtr)[buffSize - 4] = 'E';
        ((char *)appBufPtr)[buffSize - 3] = 'N';
        ((char *)appBufPtr)[buffSize - 2] = 'D';
        ((char *)appBufPtr)[buffSize - 1] = '\0';

        CacheP_wbInv((const void *)appBufPtr, buffSize);

        /*push to tx queue*/
        retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gSpiUdmaTxChObj),
                                   Udma_appVirtToPhyFxn(pHpdMem));
        if (UDMA_SOK != retVal)
        {
            SPI_log("%s:Udma_ringQueueRaw fail\r\n", __func__);
            goto TESTEND;
        }
    }
    occ = Udma_ringGetForwardRingOcc(Udma_chGetFqRingHandle(&gSpiUdmaTxChObj));
    SPI_log("%s:tx fq occ %d \n", __func__, occ);
    Udma_chResume(&gSpiUdmaTxChObj);
    Osal_delay(10);
    McSPICSDeAssert(masterParam.baseAddr, masterParam.chNum);
    McSPICSDeAssert(slaveParam.baseAddr, slaveParam.chNum);

    SPI_log("\n");
    occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gSpiUdmaTxChObj));
    SPI_log("%s:tx cq occ %d \n", __func__, occ);
    occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gSpiUdmaRxChObj));
    SPI_log("%s:rx cq occ %d \n", __func__, occ);
    SPI_log("\n");

    retVal = Udma_chGetStats((Udma_ChHandle)(&gSpiUdmaTxChObj), &chStats);
    if (UDMA_SOK == retVal)
    {
        SPI_log("%s:Completed packet count         : %d\n", __func__, chStats.packetCnt);
        SPI_log("%s:Completed payload byte count   : %d\n", __func__, chStats.completedByteCnt);
        SPI_log("%s:Started byte count             : %d\n", __func__, chStats.startedByteCnt);
    }

TESTEND:
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s:test failed!!\n", __func__);
    }
    else
    {
        if (Verify_Data(&gTxBuffer[0][0], &gRxSlvBuffer[0][0]) == true)
        {
            SPI_log("%s:Test Passed\n", __func__);
        }
        else
        {
            SPI_log("%s:Test Fail\n", __func__);
        }
    }
}
