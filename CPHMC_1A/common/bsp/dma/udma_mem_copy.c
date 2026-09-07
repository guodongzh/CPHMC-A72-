/**
 *************************************************************************
 * @file      dma.c
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     memory to memory dma test
 *************************************************************************
 */

#include "udma_mem_copy.h"

/*
 * UDMA driver objects
 */
struct Udma_ChObj gUdmaChObj;

/*
 * UDMA Memories
 * ring storage descriptor address
 */
static uint8_t gTxRingMem[UDMA_TEST_APP_RING_MEM_SIZE_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t gTxCompRingMem[UDMA_TEST_APP_RING_MEM_SIZE_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t gTxTdCompRingMem[UDMA_TEST_APP_RING_MEM_SIZE_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t gUdmaTrpdMem[UDMA_TEST_APP_LOOP_CNT][UDMA_TRPD_SIZE_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

/*
 * Application Buffers
 */
static uint8_t gUdmaTestSrcBuf[UDMA_TEST_APP_LOOP_CNT][UDMA_TEST_APP_NUM_BYTES_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t gUdmaTestDestBuf[UDMA_TEST_APP_LOOP_CNT][UDMA_TEST_APP_NUM_BYTES_ALIGN] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

/* Global test pass/fail flag */
static volatile int32_t gUdmaAppResult = UDMA_SOK;

static void udma_trpd_init_all()
{
    CSL_UdmapCppi5TRPD *pTrpd = NULL;
    CSL_UdmapTR15 *pTr = NULL;
    uint32_t *pTrResp = NULL;
    uint32_t cqRingNum = Udma_chGetCqRingNum(&gUdmaChObj);

    for (uint32_t i = 0U; i < UDMA_TEST_APP_LOOP_CNT; i++)
    {
        pTrpd = (CSL_UdmapCppi5TRPD *)&gUdmaTrpdMem[i][0U];
        pTr = (CSL_UdmapTR15 *)(&gUdmaTrpdMem[i][0U] + sizeof(CSL_UdmapTR15));
        pTrResp = (uint32_t *)(&gUdmaTrpdMem[i][0U] + (sizeof(CSL_UdmapTR15) * 2U));

        /* Make TRPD */
        UdmaUtils_makeTrpd(pTrpd, UDMA_TR_TYPE_15, 1U, cqRingNum);

        /* Setup TR */
        pTr->flags =
            CSL_FMK(UDMAP_TR_FLAGS_TYPE, 15) | CSL_FMK(UDMAP_TR_FLAGS_STATIC, 0U) |
            CSL_FMK(UDMAP_TR_FLAGS_EOL, 0U) | /* NA */
            CSL_FMK(UDMAP_TR_FLAGS_EVENT_SIZE, CSL_UDMAP_TR_FLAGS_EVENT_SIZE_COMPLETION) |
            CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE) |
            CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL) |
            CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE) |
            CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1_TYPE, CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL) |
            CSL_FMK(UDMAP_TR_FLAGS_CMD_ID, 0x25U) | /* This will come back in TR response */
            CSL_FMK(UDMAP_TR_FLAGS_SA_INDIRECT, 0U) | CSL_FMK(UDMAP_TR_FLAGS_DA_INDIRECT, 0U) |
            CSL_FMK(UDMAP_TR_FLAGS_EOP, 1U);
        pTr->icnt0 = 0;  // length
        pTr->icnt1 = 1U;
        pTr->icnt2 = 1U;
        pTr->icnt3 = 1U;
        pTr->dim1 = pTr->icnt0;
        pTr->dim2 = (pTr->icnt0 * pTr->icnt1);
        pTr->dim3 = (pTr->icnt0 * pTr->icnt1 * pTr->icnt2);
        pTr->addr = 0;               // src_addr
        pTr->fmtflags = 0x00000000U; /* Linear addressing, 1 byte per elem.
                               Replace with CSL-FL API */
        pTr->dicnt0 = 0;             // length
        pTr->dicnt1 = 1U;
        pTr->dicnt2 = 1U;
        pTr->dicnt3 = 1U;
        pTr->ddim1 = pTr->dicnt0;
        pTr->ddim2 = (pTr->dicnt0 * pTr->dicnt1);
        pTr->ddim3 = (pTr->dicnt0 * pTr->dicnt1 * pTr->dicnt2);
        pTr->daddr = 0;  // dest_addr

        /* Clear TR response memory */
        *pTrResp = 0xFFFFFFFFU;

        /* Writeback cache */
        CacheP_wb(&gUdmaTrpdMem[i][0U], UDMA_TRPD_SIZE_ALIGN);

        /* Submit TRPD to cq ring */
        Udma_ringQueueRaw(gUdmaChObj.cqRing, Udma_appVirtToPhyFxn(pTrpd));
    }
}

static void Udma_TrpdInit(Udma_ChHandle chHandle,
                          uint8_t *pTrpdMem,
                          uint64_t dest_addr,
                          uint64_t src_addr,
                          uint32_t length)
{
    CSL_UdmapCppi5TRPD *pTrpd = (CSL_UdmapCppi5TRPD *)pTrpdMem;
    CSL_UdmapTR15 *pTr = (CSL_UdmapTR15 *)(pTrpdMem + sizeof(CSL_UdmapTR15));
    uint32_t *pTrResp = (uint32_t *)(pTrpdMem + (sizeof(CSL_UdmapTR15) * 2U));
    uint32_t cqRingNum = gUdmaChObj.cqRing->ringNum;;

    /* Make TRPD */
    UdmaUtils_makeTrpd(pTrpd, UDMA_TR_TYPE_15, 1U, cqRingNum);

    pTr->icnt0 = length;  // length
    pTr->dim1 = length;
    pTr->dim2 = length;
    pTr->dim3 = length;
    pTr->addr = src_addr;

    pTr->dicnt0 = length;
    pTr->ddim1 = length;
    pTr->ddim2 = length;
    pTr->ddim3 = length;
    pTr->daddr = dest_addr;

    /* Clear TR response memory */
    *pTrResp = 0xFFFFFFFFU;

    /* Writeback cache */
    CacheP_wb(pTrpdMem, UDMA_TRPD_SIZE_ALIGN);
}

volatile uint32_t umda_fail_count = 0;

int32_t udma_memcpy(void *destBuf, void *srcBuf, uint32_t length)
{
    int32_t retVal = UDMA_SOK;
    uint8_t *trpdMem;
    uint64_t dest_addr, src_addr;
    Udma_ChHandle chHandle = &gUdmaChObj;
    uint64_t phyDescAddr;
    uint32_t cq_occ, time_out_count = 0x1000000;

    while (1)
    {
        cq_occ = Udma_ringGetForwardRingOcc(gUdmaChObj.cqRing);
        if (cq_occ != 0)
        {
            retVal = Udma_ringDequeueRaw(gUdmaChObj.cqRing, &phyDescAddr);
            if (UDMA_SOK != retVal)
            {
                DMA_log("%s:Udma_ringDequeueRaw fail %d\r\n", __func__, retVal);
                DMA_log("\r\n");
                return (retVal);
            }
            break;
        }
        if (time_out_count-- == 0)
        {
            DMA_log("[Error] Time out!!\n");
            retVal = UDMA_EFAIL;
            umda_fail_count++;
            return (retVal);
        }
    }

    /* Update TR packet descriptor */
    trpdMem = Udma_appPhyToVirtFxn(phyDescAddr);
    src_addr = Udma_appVirtToPhyFxn(srcBuf);
    dest_addr = Udma_appVirtToPhyFxn(destBuf);
    Udma_TrpdInit(chHandle, trpdMem, dest_addr, src_addr, length);

    /* Submit TRPD to channel */
    retVal = Udma_ringQueueRaw(gUdmaChObj.fqRing, Udma_appVirtToPhyFxn(trpdMem));

    return (retVal);
}

int32_t udma_memcpy_wait_complete(uint32_t time_out)
{
    int32_t retVal = UDMA_SOK;
    uint32_t cq_occ, time_out_count = time_out;
    uint32_t time_count_reload = 100, time_count = time_count_reload;
    while (1)
    {
        cq_occ = Udma_ringGetForwardRingOcc(gUdmaChObj.cqRing);
        if (cq_occ == UDMA_TEST_APP_RING_ENTRIES)
        {
            break;
        }
        time_count = time_count_reload;
        do
        {
            time_count--;
        }while (time_count != 0);
        if (time_out_count-- == 0)
        {
            DMA_log("[Error] Time out!!\n");
            retVal = UDMA_EFAIL;
            umda_fail_count++;
            return (retVal);
        }
    }
    return (retVal);
}

static void App_udmaEventTdCb(Udma_EventHandle eventHandle, uint32_t eventType, void *appData)
{
    int32_t retVal;
    CSL_UdmapTdResponse tdResp;

    if (UDMA_EVENT_TYPE_TEARDOWN_PACKET == eventType)
    {
        /* Response received in Teardown completion queue */
        retVal = Udma_chDequeueTdResponse(&gUdmaChObj, &tdResp);
        if (UDMA_SOK != retVal)
        {
            /* [Error] No TD response after callback!! */
            gUdmaAppResult = UDMA_EFAIL;
        }
    }
    else
    {
        gUdmaAppResult = UDMA_EFAIL;
    }
}

static int32_t udma_create(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle)
{
    int32_t retVal = UDMA_SOK;
    uint32_t chType;
    Udma_ChPrms chPrms;
    Udma_ChTxPrms txPrms;
    Udma_ChRxPrms rxPrms;
    Udma_EventHandle tdCqEventHandle;
    Udma_EventPrms tdCqEventPrms;

    /* Init channel parameters */
    chType = UDMA_CH_TYPE_TR_BLK_COPY;
    UdmaChPrms_init(&chPrms, chType);
    chPrms.fqRingPrms.ringMem = &gTxRingMem[0U];
    chPrms.fqRingPrms.ringMemSize = UDMA_TEST_APP_RING_MEM_SIZE;
    chPrms.fqRingPrms.elemCnt = UDMA_TEST_APP_RING_ENTRIES;

    chPrms.cqRingPrms.ringMem = &gTxCompRingMem[0U];
    chPrms.cqRingPrms.ringMemSize = UDMA_TEST_APP_RING_MEM_SIZE;
    chPrms.cqRingPrms.elemCnt = UDMA_TEST_APP_RING_ENTRIES;

    chPrms.tdCqRingPrms.ringMem = &gTxTdCompRingMem[0U];
    chPrms.tdCqRingPrms.ringMemSize = UDMA_TEST_APP_RING_MEM_SIZE;
    chPrms.tdCqRingPrms.elemCnt = UDMA_TEST_APP_RING_ENTRIES;

    /* Open channel for block copy */
    retVal = Udma_chOpen(drvHandle, chHandle, chType, &chPrms);
    if (UDMA_SOK != retVal)
    {
        DMA_log("[Error] UDMA channel open failed!!\n");
    }

    if (UDMA_SOK == retVal)
    {
        /* Config TX channel */
        UdmaChTxPrms_init(&txPrms, chType);
        retVal = Udma_chConfigTx(chHandle, &txPrms);
        if (UDMA_SOK != retVal)
        {
            DMA_log("[Error] UDMA TX channel config failed!!\n");
        }
    }

    if (UDMA_SOK == retVal)
    {
        /* Config RX channel - which is implicitly paired to TX channel in
         * block copy mode */
        UdmaChRxPrms_init(&rxPrms, chType);
        retVal = Udma_chConfigRx(chHandle, &rxPrms);
        if (UDMA_SOK != retVal)
        {
            DMA_log("[Error] UDMA RX channel config failed!!\n");
        }
    }

    if (UDMA_SOK == retVal)
    {
        /* Channel enable */
        retVal = Udma_chEnable(chHandle);
        if (UDMA_SOK != retVal)
        {
            DMA_log("[Error] UDMA channel enable failed!!\n");
        }
    }

    return (retVal);
}

static int32_t Udma_delete(Udma_DrvHandle drvHandle, Udma_ChHandle chHandle)
{
    int32_t retVal, tempRetVal;
    uint64_t pDesc;

    retVal = Udma_chDisable(chHandle, UDMA_DEFAULT_CH_DISABLE_TIMEOUT);
    if (UDMA_SOK != retVal)
    {
        DMA_log("[Error] UDMA channel disable failed!!\n");
    }

    /* Flush any pending request from the free queue */
    while (1)
    {
        tempRetVal = Udma_ringFlushRaw(Udma_chGetFqRingHandle(chHandle), &pDesc);
        if (UDMA_ETIMEOUT == tempRetVal)
        {
            break;
        }
    }

    retVal += Udma_chClose(chHandle);
    if (UDMA_SOK != retVal)
    {
        DMA_log("[Error] UDMA channel close failed!!\n");
    }
    return (retVal);
}

int32_t udma_setup(void)
{
    int32_t retVal = UDMA_SOK;
    Udma_DrvHandle drvHandle = &gUdmaDrvObj;
    Udma_ChHandle chHandle = &gUdmaChObj;

    retVal = udma_create(drvHandle, chHandle);
    if (UDMA_SOK != retVal)
    {
        DMA_log("[Error] UDMA App create failed!!\n");
    }
    else
    {
        udma_trpd_init_all();
    }
    return (retVal);
}

/*
 * UDMA memcpy test
 */
int32_t udma_memcpy_test(void)
{
    int32_t retVal = UDMA_SOK;
    uint32_t loopCnt = 0U, test_cnt = 0;
    int i = 0;
    DMA_log("\nUDMA memcpy application started...\n");
    while (test_cnt < UDMA_TEST_APP_LOOP_CNT * 2)
    {
        if (UDMA_SOK == retVal)
        {
            uint8_t *srcBuf, *destBuf;
            /* Init buffers */
            srcBuf = &gUdmaTestSrcBuf[loopCnt][0U];
            destBuf = &gUdmaTestDestBuf[loopCnt][0U];
            for (i = 0U; i < UDMA_TEST_APP_NUM_BYTES; i++)
            {
                srcBuf[i] = i;
                destBuf[i] = 0U;
            }
            /* Writeback source and destination buffer */
            CacheP_wb(srcBuf, UDMA_TEST_APP_NUM_BYTES);
            CacheP_wb(destBuf, UDMA_TEST_APP_NUM_BYTES);
            retVal = udma_memcpy(destBuf, srcBuf, UDMA_TEST_APP_NUM_BYTES);
            udma_memcpy_wait_complete(0xffffffff);
            if (UDMA_SOK == retVal)
            {
                /* Compare data */
                /* Invalidate destination buffer */
                CacheP_Inv(destBuf, UDMA_TEST_APP_NUM_BYTES);
                for (i = 0U; i < UDMA_TEST_APP_NUM_BYTES; i++)
                {
                    if (srcBuf[i] != destBuf[i])
                    {
                        DMA_log("[Error] Data mismatch!!\n");
                        retVal = UDMA_EFAIL;
                        break;
                    }
                }
            }
            if (UDMA_SOK != retVal)
            {
                DMA_log("[Error] UDMA App memcpy test failed!!\n");
            }
            else
            {
                Udma_ChStats chStats;
                retVal = Udma_chGetStats(&gUdmaChObj, &chStats);
                if (UDMA_SOK == retVal)
                {
                    DMA_log("UDMA App memcpy test statistics:\n");
                    DMA_log("Completed packet count       : %d\n", chStats.packetCnt);
                    DMA_log("Completed payload byte count : %d\n", chStats.completedByteCnt);
                    DMA_log("Started byte count           : %d\n", chStats.startedByteCnt);
                }
            }
        }

        loopCnt++;
        if (loopCnt >= UDMA_TEST_APP_LOOP_CNT)
            loopCnt = 0U;
        test_cnt++;
    }
    if ((UDMA_SOK == retVal) && (UDMA_SOK == gUdmaAppResult))
    {
        DMA_log("UDMA memcpy using TR15 block copy Passed!!\n");
    }
    else
    {
        DMA_log("UDMA memcpy using TR15 block copy Failed!!\n");
    }

    return (0);
}

