/**
 *************************************************************************
 * @file      uart.c
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     uart read and write test
 *************************************************************************
 */


#include "uart_test.h"

/* ring setting */
#define RING_ELEMENT_CNT  (10U)
#define RING_ELEMENT_SIZE (8U) /* 8 bytes */

/* APP */
#define BUF_CNT  RING_ELEMENT_CNT
#define BUF_SIZE (128u)

/* UDMA Ring Memory Pointer */
void *gUartTxFqRingMem               = NULL;
void *gUartTxCqRingMem               = NULL;
void *gUartTxTdRingMem               = NULL;
void *gUartTxPdMem[RING_ELEMENT_CNT] = {NULL};

void *gUartRxFqRingMem               = NULL;
void *gUartRxCqRingMem               = NULL;
void *gUartRxTdRingMem               = NULL;
void *gRxPdMem[RING_ELEMENT_CNT] = {NULL};

/*
 * UDMA driver objects
 */
struct Udma_ChObj gUartUdmaTxChObj;
struct Udma_ChObj gUartUdmaRxChObj;
struct Udma_EventObj gUartUdmaTxCqEventObj;
struct Udma_EventObj gUartUdmaRxCqEventObj;


/* input clock of mcu uart instance is 96MHz.
   Applicable only for mcu uart instance of am65xx, J721e and J7200 */
#define UART_MODULE_MCU_INPUT_CLK (96000000U)
/* input clock of other uart instances is 48MHz. */
#define UART_MODULE_INPUT_CLK (48000000U)



/**
 * @brief uart parameter
 */
#define UART_TEST_BASE CSL_UART2_BASE
#define BAUD_RATE      BAUD_RATE_115200
#define WORD_LENGTH    UART_WORD_LENGTH_8
#define STOP_BIT       UART_STOP_BIT_1
#define PARITY         UART_NO_PARITY
#define UART_MODE      UART_16x_MODE

/*APP*/
uint8_t uart_tx_buf[BUF_CNT][BUF_SIZE];
uint8_t uart_rx_buf[BUF_CNT][BUF_SIZE];

/**
 * @brief uart register config
 * @param uartParams uart params
 */
void UART_Config_Init(UART_Params_t uartParams)
{
    uint32_t divisorValue, fifoConfig;
    uint32_t inputClock = UART_MODULE_INPUT_CLK;

    if (uartParams.baseAddr == CSL_MCU_UART0_BASE)
    {
        inputClock = UART_MODULE_MCU_INPUT_CLK;
    }

    /* Performing a module reset. */
    UARTModuleReset(uartParams.baseAddr);

    /* Performing FIFO configurations. */
    /*
    ** - Transmit Trigger Level Granularity is 4
    ** - Receiver Trigger Level Granularity is 1
    ** - Transmit FIFO Space Setting is 56. Hence TX Trigger level
    **   is 8 (64 - 56). The TX FIFO size is 64 bytes.
    ** - The Receiver Trigger Level is 1.
    ** - Clear the Transmit FIFO.
    ** - Clear the Receiver FIFO.
    ** - DMA Mode enabling shall happen through SCR register.
    */
    fifoConfig = UART_FIFO_CONFIG(uartParams.txGra,
                                  uartParams.rxGra,
                                  uartParams.txTrigLevel,
                                  uartParams.txTrigLevel,
                                  1,
                                  1,
                                  UART_DMA_EN_PATH_FCR,
                                  uartParams.dmaMode);

    /* Configuring the FIFO settings. */
    UARTFIFOConfig(uartParams.baseAddr, fifoConfig);
    /* Performing Baud Rate settings. */
    /* Computing the Divisor Value. */
    divisorValue = UARTDivisorValCompute(inputClock,
                                         uartParams.baudRate,
                                         uartParams.mode,
                                         UART_MIR_OVERSAMPLING_RATE_42);
    /* Programming the Divisor Latches. */
    UARTDivisorLatchWrite(uartParams.baseAddr, divisorValue);

    /* Programming the Line Characteristics. */
    UARTLineCharacConfig(uartParams.baseAddr, (uartParams.wordLength | uartParams.stopBit), uartParams.parity);

    /* Disabling write access to Divisor Latches. */
    UARTDivisorLatchDisable(uartParams.baseAddr);

    /* Disabling Break Control. */
    UARTBreakCtl(uartParams.baseAddr, UART_BREAK_COND_DISABLE);

    UARTLoopbackModeControl(uartParams.baseAddr, UART_LOOPBACK_MODE_DISABLE);

    /* Uart enable */
    UARTOperatingModeSelect(uartParams.baseAddr, uartParams.mode);
}

/**
 * @brief uart send string
 * @param baseAddr uart peripheral base address
 * @param pTxBuffer pointer to the buffer containing the data to be transmitted
 * @param numBytesToWrite number of bytes to be transmitted
 * @return number of bytes transmitted
 */
uint32_t UART_Puts(uint32_t baseAddr, const char *pTxBuffer, int32_t numBytesToWrite)
{
    uint32_t count = 0U;
    uint32_t flag  = 0U;

    if (numBytesToWrite < 0)
    {
        flag = 1U;
    }

    while ('\0' != *pTxBuffer)
    {
        /* Checks if data is a newline character. */
        if ('\n' == *pTxBuffer)
        {
            /* Ensuring applicability to serial console.*/
            UARTCharPut(baseAddr, '\r');
            UARTCharPut(baseAddr, '\n');
        }
        else
        {
            UARTCharPut(baseAddr, (uint8_t)*pTxBuffer);
        }
        pTxBuffer++;
        count++;
        if ((0U == flag) && (count == (uint32_t)numBytesToWrite))
        {
            break;
        }
    }
    /* Returns the number of bytes written onto the transmitter FIFO. */
    return count;
}

/**
 * @brief uart receive string
 * @param baseAddr uart peripheral base address
 * @param pRxBuffer ponter to the buffer to which the received data has to be copied
 * @param numBytesToRead number of bytes to be read
 * @note  the string must end with a newline character ('\n') or a Carriage Return ('\r')
 * @return number of bytes read
 */
uint32_t UART_Gets(uint32_t baseAddr, char *pRxBuffer, int32_t numBytesToRead)
{
    int32_t count = 0;
    uint32_t flag = 0U;
    uint8_t tmp;

    if (numBytesToRead < 0)
    {
        flag = 1U;
    }
    do
    {
        *pRxBuffer = (char)UARTCharGet(baseAddr);

        /*
        ** 0xD - ASCII value of Carriage Return.
        ** 0x1B - ASCII value of ESC character.
        */
        tmp = (uint8_t)*pRxBuffer;
        if ('\n' == tmp || '\r' == tmp)
        {
            *pRxBuffer = '\0';
            break;
        }

        /* Echoing the typed character back to the serial console. */
        UARTCharPut(baseAddr, (uint8_t)*pRxBuffer);
        pRxBuffer++;
        count++;
        if (0U == flag && (count == numBytesToRead))
        {
            break;
        }
    } while (1U);

    return count;
}


/**
 * @brief Configuring UDMA for uart transmit
 * @return UDMA_SOK if success, else error code
 */
int32_t UART_TX_UDMA_Config(UART_DmaParams_t *params)
{
    uint32_t chType;
    int32_t retVal;
    Udma_ChTxPrms txPrms;
    Udma_ChPrms chPrms;

    /* Init TX channel parameters */
    if (params == NULL || params->fqRingMem == NULL || params->cqRingMem == NULL || params->tdRingMem == NULL)
    {
        retVal = UDMA_EALLOC;
        return retVal;
    }
    chType = UDMA_CH_TYPE_PDMA_TX;
    UdmaChPrms_init(&chPrms, chType);
    chPrms.peerChNum            = params->peerChNum;
    chPrms.fqRingPrms.ringMem   = params->fqRingMem;
    chPrms.cqRingPrms.ringMem   = params->cqRingMem;
    chPrms.tdCqRingPrms.ringMem = params->tdRingMem;
    chPrms.fqRingPrms.elemCnt   = params->elemCnt;
    chPrms.cqRingPrms.elemCnt   = params->elemCnt;
    chPrms.tdCqRingPrms.elemCnt = params->elemCnt;
    retVal                      = Udma_chOpen(params->drvHandle, params->chHandle, chType, &chPrms);
    if (retVal != UDMA_SOK)
    {
        UART_log("UART_TX_UDMA_Config: UDMA TX channel open failed!!\n");
        return retVal;
    }
    UdmaChTxPrms_init(&txPrms, chType);
    retVal = Udma_chConfigTx(params->chHandle, &txPrms);
    if (retVal != UDMA_SOK)
    {
        UART_log("UART_TX_UDMA_Config: UDMA TX channel config failed!!\n");
        return retVal;
    }
    return retVal;
}

/**
 * @brief Configuring UDMA for uart receive
 * @return UDMA_SOK if success, else error code
 */
int32_t UART_RX_UDMA_Config(UART_DmaParams_t params)
{
    uint32_t chType;
    int32_t retVal;
    Udma_ChRxPrms rxPrms;
    Udma_ChPrms chPrms;

    /* Init TX channel parameters */
    if (params.fqRingMem == NULL || params.cqRingMem == NULL || params.tdRingMem == NULL)
    {
        retVal = UDMA_EALLOC;
        return retVal;
    }
    chType = UDMA_CH_TYPE_PDMA_RX;
    UdmaChPrms_init(&chPrms, chType);
    chPrms.peerChNum            = params.peerChNum;
    chPrms.fqRingPrms.ringMem   = params.fqRingMem;
    chPrms.cqRingPrms.ringMem   = params.cqRingMem;
    chPrms.tdCqRingPrms.ringMem = params.tdRingMem;
    chPrms.fqRingPrms.elemCnt   = params.elemCnt;
    chPrms.cqRingPrms.elemCnt   = params.elemCnt;
    chPrms.tdCqRingPrms.elemCnt = params.elemCnt;
    retVal                      = Udma_chOpen(params.drvHandle, params.chHandle, chType, &chPrms);
    if (retVal != UDMA_SOK)
    {
        UART_log("UART_RX_UDMA_Config: UDMA TX channel open failed!!\n");
        return retVal;
    }
    UdmaChRxPrms_init(&rxPrms, chType);
    retVal = Udma_chConfigRx(params.chHandle, &rxPrms);
    if (retVal != UDMA_SOK)
    {
        UART_log("UART_RX_UDMA_Config: UDMA TX channel config failed!!\n");
        return retVal;
    }
    return retVal;
}

/**
 * @brief UART CSL task
 * @param arg0 task param
 * @param arg1 task param
 */
void UART_CSL_Task(void *arg0, void *arg1)
{
    uint32_t uartBaseAddr = UART_TEST_BASE;
    char dataBuffer[100];

    /* config uart */
    UART_Params_t uartParams;
    uartParams.baseAddr    = UART_TEST_BASE;
    uartParams.baudRate    = BAUD_RATE;
    uartParams.wordLength  = WORD_LENGTH;
    uartParams.stopBit     = STOP_BIT;
    uartParams.parity      = PARITY;
    uartParams.mode        = UART_MODE;
    uartParams.dmaMode     = UART_DMA_MODE_0_ENABLE;
    uartParams.txGra       = UART_TRIG_LVL_GRANULARITY_4;
    uartParams.rxGra       = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.txTrigLevel = 56;  /* FIFO transmit space setting */
    uartParams.rxTrigLevel = 1;
    UART_Config_Init(uartParams);

    //Debug_log("Uart_x BaseAddr is: 0x%u\r\n", uartBaseAddr);
    UART_log("The UART base address is: 0x%08X\n", uartBaseAddr);
    UART_log("The UART2 Test begin\n");
    UART_Puts(uartBaseAddr, "\nUART Console Test App", -1);
    UART_Puts(uartBaseAddr, "\nUser needs to enter input string", -1);
    UART_Puts(uartBaseAddr, "\nOutput can be observed on UART console ", -1);
    UART_Puts(uartBaseAddr, "\nTest is marked as PASS if input and output matches", -1);
    UART_Puts(uartBaseAddr, "\nTest uses below parameters: ", -1);
    UART_Puts(uartBaseAddr, "\n  -BAUD_RATE    = 115200", -1);
    UART_Puts(uartBaseAddr, "\n  -WORD_LENGTH  = 8Bits", -1);
    UART_Puts(uartBaseAddr, "\n  -STOP_BIT     = 1", -1);
    UART_Puts(uartBaseAddr, "\n  -PARITY       = None", -1);
    UART_Puts(uartBaseAddr, "\n\n", -1);
    UART_Puts(uartBaseAddr, "\nUART Test has started", -1);

    /* UART receive and transmit operation */
    UART_Puts(uartBaseAddr, "\nUART Test", -1);
    UART_Puts(uartBaseAddr, "\nEnter some data to transmit:", -1);
    UART_Gets(uartBaseAddr, &dataBuffer[0], -1);
    UART_Puts(uartBaseAddr, "\nData Received:", -1);
    UART_Puts(uartBaseAddr, &dataBuffer[0], -1);
    UART_Puts(uartBaseAddr, "\nUART_CSL_Task tests have passed.", -1);
    UART_log("UART_CSL_Task Passed\n");
}

/**
 * @brief UART transmit PDMA configuration
 * @param chHandle UDMA channel handle
 * @param fifoLevel UART FIFO level
 * @param size BUffer size
 * @return UDMA_SOK if success, else error code
 */
int32_t UART_TX_PDMA_Config(Udma_ChHandle chHandle, uint32_t fifoLevel, size_t size)
{
    int32_t retVal;
    Udma_ChPdmaPrms pdmaPrms;

    UdmaChPdmaPrms_init(&pdmaPrms);
    pdmaPrms.elemSize = UDMA_PDMA_ES_8BITS; /* 8-bit */
    if ((uint32_t)size > fifoLevel)
    {
        pdmaPrms.elemCnt = fifoLevel;
    }
    else
    {
        pdmaPrms.elemCnt = (uint32_t)size;
    }
    pdmaPrms.fifoCnt = 0U; /* Don't care for write */
    retVal           = Udma_chConfigPdma(chHandle, &pdmaPrms);

    if (UDMA_SOK == retVal)
    {
        retVal = Udma_chEnable(chHandle);
        if (UDMA_SOK != retVal)
        {
            UART_log("UDMA App: UDMA TX channel enable failed!!\n");
        }
    }

    return retVal;
}

/**
 * @brief UART receive PDMA configuration
 * @param chHandle UDMA channel handle
 * @param fifoLevel UART FIFO level
 * @param size BUffer size
 * @return UDMA_SOK if success, else error code
 */
int32_t UART_RX_PDMA_Config(Udma_ChHandle chHandle, uint32_t fifoLevel, size_t size)
{
    int32_t retVal;
    uint32_t rxSize;
    Udma_ChPdmaPrms pdmaPrms;

    if ((uint32_t)size <= fifoLevel)
    {
        rxSize = (uint32_t)size;
    }
    else
    {
        rxSize = ((uint32_t)size / fifoLevel) * fifoLevel;
    }

    UdmaChPdmaPrms_init(&pdmaPrms);
    pdmaPrms.elemSize = UDMA_PDMA_ES_8BITS; /* 8-bit */
    if (rxSize > fifoLevel)
    {
        pdmaPrms.elemCnt = fifoLevel;
        pdmaPrms.fifoCnt = rxSize / fifoLevel;
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
 * @brief UART UDMA Task
 * @param arg0 UDMA Task Parameter
 * @param arg1 UDMA Task Parameter
 */
void UART_UDMA_Task(void *arg0, void *arg1)
{
    int32_t retVal = UDMA_SOK;
    uint32_t i, j;
    uint32_t buff_size = BUF_SIZE;
    CSL_UdmapCppi5HMPD *pHpdMem;
    uint32_t occ;
    Udma_ChStats chStats;
    uint64_t phyDescAddr;
    void *appBufPtr;
    UART_Params_t uartParams;
    UART_DmaParams_t dmaParams;

    UART_log("%s: Starting!!\r\n", __func__);


    /* config tx udma */
    dmaParams.drvHandle = &gUdmaDrvObj;
    dmaParams.chHandle  = &gUartUdmaTxChObj;
    dmaParams.fqRingMem = gUartTxFqRingMem;
    dmaParams.cqRingMem = gUartTxCqRingMem;
    dmaParams.tdRingMem = gUartTxTdRingMem;
    dmaParams.elemCnt   = RING_ELEMENT_CNT;
    dmaParams.peerChNum = CSL_PDMA_CH_MAIN_UART0_CH0_TX;
    retVal              = UART_TX_UDMA_Config(&dmaParams);
    if (UDMA_SOK != retVal)
    {
        UART_log("%s: UDMA config failed\r\n", __func__);
        goto TESTEND;
    }

    /* config uart */
    uartParams.baseAddr    = UART_TEST_BASE;
    uartParams.baudRate    = BAUD_RATE;
    uartParams.wordLength  = WORD_LENGTH;
    uartParams.stopBit     = STOP_BIT;
    uartParams.parity      = PARITY;
    uartParams.mode        = UART_MODE;
    uartParams.dmaMode     = UART_DMA_MODE_1_ENABLE;
    uartParams.txGra       = UART_TRIG_LVL_GRANULARITY_4;
    uartParams.rxGra       = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.txTrigLevel = 32;
    uartParams.rxTrigLevel = 1;
    UART_Config_Init(uartParams);

    /* config pdma */
    retVal = UART_TX_PDMA_Config(&gUartUdmaTxChObj, uartParams.txTrigLevel, buff_size);
    if (UDMA_SOK != retVal)
    {
        UART_log("%s: UDMA config failed\r\n", __func__);
        goto TESTEND;
    }

    /* build host packet descriptor */
    for (i = 0; i < RING_ELEMENT_CNT; ++i)
    {
        pHpdMem = gUartTxPdMem[i];
        if (NULL == pHpdMem)
        {
            UART_log("Failed to allocate memory for TX HPD\n");
            goto TESTEND;
        }
        UDMA_Hpd_Init(&gUartUdmaTxChObj, pHpdMem, Udma_appVirtToPhyFxn(uart_tx_buf[i]), buff_size);
        retVal = Udma_ringQueueRaw(Udma_chGetCqRingHandle(&gUartUdmaTxChObj), Udma_appVirtToPhyFxn(pHpdMem));
        if (retVal != UDMA_SOK)
        {
            UART_log("Failed to submit TX FQ code: %d \n", retVal);
            goto TESTEND;
        }
    }

    occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gUartUdmaTxChObj));
    UART_log("tx cq occ %d \n", occ);

    /* pop dp from completion queue, fill data, push to forward queue*/
    for (i = 0; i < RING_ELEMENT_CNT; ++i)
    {
        retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gUartUdmaTxChObj), &phyDescAddr);
        if (UDMA_SOK != retVal)
        {
            UART_log("Udma_ringDequeueRaw fail %d\r\n", retVal);
            UART_log("\r\n");
            goto TESTEND;
        }
        pHpdMem = (CSL_UdmapCppi5HMPD *)Udma_appPhyToVirtFxn(phyDescAddr);

        CacheP_wbInv((const void *)pHpdMem, (int32_t)(sizeof(CSL_UdmapCppi5HMPD)));

        /*user write buffer*/
        appBufPtr = Udma_appPhyToVirtFxn(pHpdMem->orgBufPtr);
        memset(appBufPtr, 0, buff_size);
        ((char *)appBufPtr)[0] = '\n';
        ((char *)appBufPtr)[1] = 'B';
        ((char *)appBufPtr)[2] = 'E';
        ((char *)appBufPtr)[3] = 'G';
        ((char *)appBufPtr)[4] = 'I';
        ((char *)appBufPtr)[5] = 'N';
        ((char *)appBufPtr)[6] = '\n';
        for (j = 7; j < (buff_size - 5); j++)
        {
            ((char *)appBufPtr)[j] = '0' + i;
        }
        ((char *)appBufPtr)[buff_size - 5] = '\n';
        ((char *)appBufPtr)[buff_size - 4] = 'E';
        ((char *)appBufPtr)[buff_size - 3] = 'N';
        ((char *)appBufPtr)[buff_size - 2] = 'D';
        ((char *)appBufPtr)[buff_size - 1] = '\0';

        CacheP_wbInv((const void *)appBufPtr, buff_size);

        /*push to tx queue*/
        retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gUartUdmaTxChObj), Udma_appVirtToPhyFxn(pHpdMem));
        if (UDMA_SOK != retVal)
        {
            UART_log("Udma_ringQueueRaw fail\r\n");
            goto TESTEND;
        }
    }
    occ = Udma_ringGetForwardRingOcc(Udma_chGetFqRingHandle(&gUartUdmaTxChObj));
    UART_log("%s:rx cq occ %d \n", __func__, occ);

    occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gUartUdmaTxChObj));
    UART_log("tx cq occ %d \n", occ);

    retVal = Udma_chGetStats((Udma_ChHandle)(&gUartUdmaTxChObj), &chStats);
    if (UDMA_SOK == retVal)
    {
        UART_log("%s:Completed packet count         : %d\n", __func__, chStats.packetCnt);
        UART_log("%s:Completed payload byte count   : %d\n", __func__, chStats.completedByteCnt);
        UART_log("%s:Started byte count             : %d\n", __func__, chStats.startedByteCnt);
    }

TESTEND:
    if (UDMA_SOK != retVal)
    {
        UART_log("%s:Test Fail\n", __func__);
    }
    else
    {
        UART_log("%s:Test Passed\n", __func__);
    }
}

void uart_init_all()
{
    UART_Params_t uartParams;
    uartParams.baudRate    = BAUD_RATE_115200;
    uartParams.wordLength  = UART_WORD_LENGTH_8;
    uartParams.stopBit     = UART_STOP_BIT_1;
    uartParams.parity      = UART_NO_PARITY;
    uartParams.mode        = UART_16x_MODE;
    uartParams.dmaMode     = UART_DMA_MODE_0_ENABLE;
    uartParams.txGra       = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.rxGra       = UART_TRIG_LVL_GRANULARITY_1;
    uartParams.txTrigLevel = 1;
    uartParams.rxTrigLevel = 1;

    uartParams.baseAddr    = CSL_UART2_BASE;
    UART_Config_Init(uartParams);

    uartParams.baseAddr    = CSL_UART3_BASE;
    UART_Config_Init(uartParams);

    uartParams.baseAddr    = CSL_UART4_BASE;
    UART_Config_Init(uartParams);

    uartParams.baseAddr    = CSL_UART5_BASE;
    UART_Config_Init(uartParams);

    uartParams.baseAddr    = CSL_UART6_BASE;
    UART_Config_Init(uartParams);
}


void uart_test_all()
{
    UART_Puts(CSL_UART2_BASE, "UART2 print\n", -1);
    UART_Puts(CSL_UART3_BASE, "UART3 print\n", -1);
    UART_Puts(CSL_UART4_BASE, "UART4 print\n", -1);
    UART_Puts(CSL_UART5_BASE, "UART5 print\n", -1);
    UART_Puts(CSL_UART6_BASE, "UART6 print\n", -1);
}


