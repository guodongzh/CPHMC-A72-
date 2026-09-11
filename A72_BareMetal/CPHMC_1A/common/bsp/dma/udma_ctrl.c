/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       udma_ctrl.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      UDMA block-copy control implementation for A72 No-OS.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include "bsp/dma/udma_ctrl.h"

#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/udma/udma.h>
#include <ti/drv/udma/examples/udma_apputils/udma_apputils.h>
#include <ti/drv/uart/UART_stdio.h>

#define UDMA_CTRL_RING_ENTRIES        (1U)
#define UDMA_CTRL_RING_ENTRY_SIZE     (sizeof(uint64_t))
#define UDMA_CTRL_RING_MEM_SIZE       \
    (UDMA_CTRL_RING_ENTRIES * UDMA_CTRL_RING_ENTRY_SIZE)
#define UDMA_CTRL_RING_MEM_SIZE_ALIGN \
    ((UDMA_CTRL_RING_MEM_SIZE + UDMA_CACHELINE_ALIGNMENT) & \
     ~(UDMA_CACHELINE_ALIGNMENT - 1U))
#define UDMA_CTRL_TRPD_SIZE           ((sizeof(CSL_UdmapTR15) * 2U) + 4U)
#define UDMA_CTRL_TRPD_SIZE_ALIGN     \
    ((UDMA_CTRL_TRPD_SIZE + UDMA_CACHELINE_ALIGNMENT) & \
     ~(UDMA_CACHELINE_ALIGNMENT - 1U))
#define UDMA_CTRL_TEST_SIZE            (1024U)
#define UDMA_CTRL_TIMEOUT_LOOPS        (10000000U)

static struct Udma_DrvObj g_udmaCtrlDrvObj;
static struct Udma_ChObj g_udmaCtrlChObj;

static uint8_t g_udmaCtrlFqRingMem[UDMA_CTRL_RING_MEM_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
#if (UDMA_SOC_CFG_RA_NORMAL_PRESENT == 1)
static uint8_t g_udmaCtrlCqRingMem[UDMA_CTRL_RING_MEM_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t g_udmaCtrlTdCqRingMem[UDMA_CTRL_RING_MEM_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
#endif
static uint8_t g_udmaCtrlTrpdMem[UDMA_CTRL_TRPD_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t g_udmaCtrlTestSrc[UDMA_CTRL_TEST_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
static uint8_t g_udmaCtrlTestDst[UDMA_CTRL_TEST_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

static int32_t g_udmaCtrlInitialized;
static int32_t g_udmaCtrlSciclientInitialized;
static int32_t g_udmaCtrlBusy;

static void udma_ctrl_print(const char *message)
{
    UART_printf("%s", message);
}

static int32_t udma_ctrl_sciclient_init(void)
{
    int32_t retVal;
    Sciclient_ConfigPrms_t sciclientCfg;

    if (g_udmaCtrlSciclientInitialized != 0)
    {
        return UDMA_SOK;
    }

    retVal = Sciclient_configPrmsInit(&sciclientCfg);
    if (retVal == CSL_PASS)
    {
        /* SBL has already loaded System Firmware and applied boardcfg. */
        sciclientCfg.skipLocalBoardCfgProcess = 1U;
        retVal = Sciclient_init(&sciclientCfg);
    }
    if (retVal == CSL_PASS)
    {
        g_udmaCtrlSciclientInitialized = 1;
    }

    return retVal;
}

static int32_t udma_ctrl_channel_create(void)
{
    int32_t retVal;
    uint32_t chType = UDMA_CH_TYPE_TR_BLK_COPY;
    Udma_ChPrms chPrms;
    Udma_ChTxPrms txPrms;
    Udma_ChRxPrms rxPrms;

    UdmaChPrms_init(&chPrms, chType);
    chPrms.fqRingPrms.ringMem = &g_udmaCtrlFqRingMem[0U];
    chPrms.fqRingPrms.ringMemSize = UDMA_CTRL_RING_MEM_SIZE;
    chPrms.fqRingPrms.elemCnt = UDMA_CTRL_RING_ENTRIES;
#if (UDMA_SOC_CFG_RA_NORMAL_PRESENT == 1)
    chPrms.cqRingPrms.ringMem = &g_udmaCtrlCqRingMem[0U];
    chPrms.cqRingPrms.ringMemSize = UDMA_CTRL_RING_MEM_SIZE;
    chPrms.cqRingPrms.elemCnt = UDMA_CTRL_RING_ENTRIES;
    chPrms.tdCqRingPrms.ringMem = &g_udmaCtrlTdCqRingMem[0U];
    chPrms.tdCqRingPrms.ringMemSize = UDMA_CTRL_RING_MEM_SIZE;
    chPrms.tdCqRingPrms.elemCnt = UDMA_CTRL_RING_ENTRIES;
#endif

    retVal = Udma_chOpen(&g_udmaCtrlDrvObj, &g_udmaCtrlChObj, chType, &chPrms);
    if (retVal == UDMA_SOK)
    {
        UdmaChTxPrms_init(&txPrms, chType);
        retVal = Udma_chConfigTx(&g_udmaCtrlChObj, &txPrms);
    }
    if (retVal == UDMA_SOK)
    {
        UdmaChRxPrms_init(&rxPrms, chType);
        retVal = Udma_chConfigRx(&g_udmaCtrlChObj, &rxPrms);
    }
    if (retVal == UDMA_SOK)
    {
        retVal = Udma_chEnable(&g_udmaCtrlChObj);
    }

    return retVal;
}

static void udma_ctrl_trpd_init(void *destination,
                                 const void *source,
                                 uint32_t length)
{
    CSL_UdmapCppi5TRPD *trpd = (CSL_UdmapCppi5TRPD *)&g_udmaCtrlTrpdMem[0U];
    CSL_UdmapTR15 *tr = (CSL_UdmapTR15 *)
        (&g_udmaCtrlTrpdMem[0U] + sizeof(CSL_UdmapTR15));
    uint32_t *trResponse = (uint32_t *)
        (&g_udmaCtrlTrpdMem[0U] + (sizeof(CSL_UdmapTR15) * 2U));
    uint32_t cqRingNum = Udma_chGetCqRingNum(&g_udmaCtrlChObj);

    UdmaUtils_makeTrpd(trpd, UDMA_TR_TYPE_15, 1U, cqRingNum);

    tr->flags =
        CSL_FMK(UDMAP_TR_FLAGS_TYPE, 15) |
        CSL_FMK(UDMAP_TR_FLAGS_STATIC, 0U) |
        CSL_FMK(UDMAP_TR_FLAGS_EOL, 0U) |
        CSL_FMK(UDMAP_TR_FLAGS_EVENT_SIZE,
                CSL_UDMAP_TR_FLAGS_EVENT_SIZE_COMPLETION) |
        CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE) |
        CSL_FMK(UDMAP_TR_FLAGS_TRIGGER0_TYPE,
                CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL) |
        CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1, CSL_UDMAP_TR_FLAGS_TRIGGER_NONE) |
        CSL_FMK(UDMAP_TR_FLAGS_TRIGGER1_TYPE,
                CSL_UDMAP_TR_FLAGS_TRIGGER_TYPE_ALL) |
        CSL_FMK(UDMAP_TR_FLAGS_CMD_ID, 0x25U) |
        CSL_FMK(UDMAP_TR_FLAGS_SA_INDIRECT, 0U) |
        CSL_FMK(UDMAP_TR_FLAGS_DA_INDIRECT, 0U) |
        CSL_FMK(UDMAP_TR_FLAGS_EOP, 1U);
    tr->icnt0 = length;
    tr->icnt1 = 1U;
    tr->icnt2 = 1U;
    tr->icnt3 = 1U;
    tr->dim1 = length;
    tr->dim2 = length;
    tr->dim3 = length;
    tr->addr = Udma_appVirtToPhyFxn(source, UDMA_DMA_CH_INVALID, (void *)0);
    tr->fmtflags = 0U;
    tr->dicnt0 = length;
    tr->dicnt1 = 1U;
    tr->dicnt2 = 1U;
    tr->dicnt3 = 1U;
    tr->ddim1 = length;
    tr->ddim2 = length;
    tr->ddim3 = length;
    tr->daddr = Udma_appVirtToPhyFxn(destination,
                                     UDMA_DMA_CH_INVALID,
                                     (void *)0);

    *trResponse = 0xFFFFFFFFU;
    Udma_appUtilsCacheWb(&g_udmaCtrlTrpdMem[0U], UDMA_CTRL_TRPD_SIZE);
}

int32_t udma_ctrl_init(void)
{
    int32_t retVal;
    Udma_InitPrms initPrms;

    if (g_udmaCtrlInitialized != 0)
    {
        return UDMA_SOK;
    }

    retVal = udma_ctrl_sciclient_init();
    if (retVal == CSL_PASS)
    {
        retVal = UdmaInitPrms_init(UDMA_INST_ID_MAIN_0, &initPrms);
    }
    if (retVal == UDMA_SOK)
    {
        initPrms.virtToPhyFxn = &Udma_appVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Udma_appPhyToVirtFxn;
        initPrms.printFxn = &udma_ctrl_print;
        retVal = Udma_init(&g_udmaCtrlDrvObj, &initPrms);
    }
    if (retVal == UDMA_SOK)
    {
        retVal = udma_ctrl_channel_create();
    }
    if (retVal == UDMA_SOK)
    {
        g_udmaCtrlInitialized = 1;
    }

    return retVal;
}

int32_t udma_ctrl_memcpy(void *destination,
                         const void *source,
                         uint32_t length,
                         uint32_t timeoutLoops)
{
    int32_t retVal;
    uint32_t loopCount;
    uint32_t *trResponse;
    uint32_t trResponseStatus;
    uint64_t completedTrpd = 0U;
    uint64_t submittedTrpd;

    if ((destination == (void *)0) || (source == (const void *)0) ||
        (length == 0U) || (timeoutLoops == 0U))
    {
        return UDMA_EBADARGS;
    }
    if (g_udmaCtrlInitialized == 0)
    {
        return UDMA_EFAIL;
    }
    if (g_udmaCtrlBusy != 0)
    {
        return UDMA_EFAIL;
    }

    g_udmaCtrlBusy = 1;
    Udma_appUtilsCacheWb(source, (int32_t)length);
    Udma_appUtilsCacheWbInv(destination, (int32_t)length);
    udma_ctrl_trpd_init(destination, source, length);

    submittedTrpd = Udma_appVirtToPhyFxn(&g_udmaCtrlTrpdMem[0U],
                                         UDMA_DMA_CH_INVALID,
                                         (void *)0);
    retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&g_udmaCtrlChObj),
                               submittedTrpd);
    if (retVal == UDMA_SOK)
    {
        retVal = UDMA_ETIMEOUT;
        for (loopCount = 0U; loopCount < timeoutLoops; loopCount++)
        {
            retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&g_udmaCtrlChObj),
                                         &completedTrpd);
            if (retVal == UDMA_SOK)
            {
                break;
            }
            if (retVal != UDMA_ETIMEOUT)
            {
                break;
            }
        }
    }
    if ((retVal == UDMA_SOK) && (completedTrpd != submittedTrpd))
    {
        retVal = UDMA_EFAIL;
    }
    if (retVal == UDMA_SOK)
    {
        Udma_appUtilsCacheInv(&g_udmaCtrlTrpdMem[0U], UDMA_CTRL_TRPD_SIZE);
        trResponse = (uint32_t *)
            (&g_udmaCtrlTrpdMem[0U] + (sizeof(CSL_UdmapTR15) * 2U));
        trResponseStatus = CSL_FEXT(*trResponse,
                                    UDMAP_TR_RESPONSE_STATUS_TYPE);
        if (trResponseStatus != CSL_UDMAP_TR_RESPONSE_STATUS_COMPLETE)
        {
            retVal = UDMA_EFAIL;
        }
    }
    if (retVal == UDMA_SOK)
    {
        Udma_appUtilsCacheInv(destination, (int32_t)length);
    }

    g_udmaCtrlBusy = 0;
    return retVal;
}

int32_t udma_ctrl_self_test(void)
{
    int32_t retVal;
    uint32_t index;

    for (index = 0U; index < UDMA_CTRL_TEST_SIZE; index++)
    {
        g_udmaCtrlTestSrc[index] = (uint8_t)(index ^ 0xA5U);
        g_udmaCtrlTestDst[index] = 0U;
    }

    retVal = udma_ctrl_memcpy(&g_udmaCtrlTestDst[0U],
                              &g_udmaCtrlTestSrc[0U],
                              UDMA_CTRL_TEST_SIZE,
                              UDMA_CTRL_TIMEOUT_LOOPS);
    if (retVal == UDMA_SOK)
    {
        for (index = 0U; index < UDMA_CTRL_TEST_SIZE; index++)
        {
            if (g_udmaCtrlTestDst[index] != g_udmaCtrlTestSrc[index])
            {
                retVal = UDMA_EFAIL;
                break;
            }
        }
    }

    return retVal;
}
