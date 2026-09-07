/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>
#include <stdio.h>
#include <ti/csl/csl_types.h>
#include <ti/csl/soc.h>
#include <ti/csl/hw_types.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/csl_mcan.h>
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/osal/osal.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>
#include <ti/csl/csl_gpio.h>
#include <ti/drv/i2c/I2C.h>
#include "mcan_test.h"
#include "debug_config.h"

/* ========================================================================== */
/*                                Macros                                      */
/* ========================================================================== */

#define APP_ENABLE_UART_PRINT           (1U)

#define APP_MCAN_STD_ID_FILT_START_ADDR (0U)
#define APP_MCAN_STD_ID_FILTER_NUM      (1U)
#define APP_MCAN_EXT_ID_FILT_START_ADDR (48U)
#define APP_MCAN_EXT_ID_FILTER_NUM      (1U)
#define APP_MCAN_TX_EVENT_START_ADDR    (100U)
#define APP_MCAN_TX_EVENT_SIZE          (5U)
#define APP_MCAN_TX_BUFF_START_ADDR     (148U)
#define APP_MCAN_TX_BUFF_SIZE           (5U)
#define APP_MCAN_TX_FIFO_SIZE           (5U)
#define APP_MCAN_FIFO_0_START_ADDR      (548U)
#define APP_MCAN_FIFO_0_NUM             (5U)
#define APP_MCAN_FIFO_1_START_ADDR      (748U)
#define APP_MCAN_FIFO_1_NUM             (5U)
#define APP_MCAN_RX_BUFF_START_ADDR     (948U)

#define APP_MCAN_EXT_ID_AND_MASK        (0x1FFFFFFFU)

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#define APP_MCU_MCAN_0_INT0 (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN0_MCANSS_MCAN_LVL_INT_0)
#define APP_MCU_MCAN_0_INT1 (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN0_MCANSS_MCAN_LVL_INT_1)
#define APP_MCU_MCAN_0_TS_INT                                                                      \
    (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN0_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)
#define APP_MCU_MCAN_1_INT0 (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN1_MCANSS_MCAN_LVL_INT_0)
#define APP_MCU_MCAN_1_INT1 (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN1_MCANSS_MCAN_LVL_INT_1)
#define APP_MCU_MCAN_1_TS_INT                                                                      \
    (CSLR_MCU_R5FSS0_CORE0_INTR_MCU_MCAN1_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)
#endif

/* Macros for MAIN MCAN Instances to use for testing */
#define APP_MAIN_MCAN_DEF_INST0_BASE_ADDRESS (CSL_MCAN4_MSGMEM_RAM_BASE)
#if defined(SOC_J721E) || defined(SOC_J7200)
#define APP_MAIN_MCAN_DEF_INST1_BASE_ADDRESS (CSL_MCAN0_MSGMEM_RAM_BASE)
#elif defined(SOC_J721S2) || defined(SOC_J784S4)
#define APP_MAIN_MCAN_DEF_INST1_BASE_ADDRESS (CSL_MCAN16_MSGMEM_RAM_BASE)
#endif

/* MCAN0 */
#define APP_MAIN_MCAN_DEF_INST0_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN0_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST0_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN0_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST0_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN0_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* MCAN1 */
#define APP_MAIN_MCAN_DEF_INST1_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN1_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST1_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN1_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST1_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN1_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* MCAN2 */
#define APP_MAIN_MCAN_DEF_INST2_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN2_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST2_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN2_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST2_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN2_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* MCAN3 */
#define APP_MAIN_MCAN_DEF_INST3_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN3_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST3_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN3_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST3_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN3_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* MCAN4 */
#define APP_MAIN_MCAN_DEF_INST4_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN4_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST4_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN4_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST4_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN4_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* MCAN5 */
#define APP_MAIN_MCAN_DEF_INST5_INT0 (CSLR_R5FSS1_CORE0_INTR_MCAN5_MCANSS_MCAN_LVL_INT_0)
#define APP_MAIN_MCAN_DEF_INST5_INT1 (CSLR_R5FSS1_CORE0_INTR_MCAN5_MCANSS_MCAN_LVL_INT_1)
#define APP_MAIN_MCAN_DEF_INST5_TS_INT (CSLR_R5FSS1_CORE0_INTR_MCAN5_MCANSS_EXT_TS_ROLLOVER_LVL_INT_0)

/* Print buffer character limit for prints- UART or CCS Console */
#define APP_PRINT_BUFFER_SIZE (4000U)

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

uint32_t gMcanAppdataSize[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
volatile uint32_t gMcan0IsrIntr0Flag = 1U, gMcan1IsrIntr0Flag = 1U;
volatile uint32_t gMcan0IsrIntr1Flag = 1U, gMcan1IsrIntr1Flag = 1U;
MCAN_ECCErrStatus gMcaneccErr;

uint32_t gMcanTxModAddr;
uint32_t gMcanRxModAddr;

/* ========================================================================== */
/*                 Internal Function Declarations                             */
/* ========================================================================== */

/**
 * \brief   This function will configure MCAN module
 *
 * \param   mcanInstAddr            MCAN Instance address
 *          enableInternalLpbk      Flag to enable/disable Internal loopback
 *
 * \retval  status      configuration status.
 */
static int32_t App_mcanConfig(uint32_t mcanInstAddr, bool enableInternalLpbk);

/**
 * \brief   This function will configure X-BAR for MCAN interrupts
 *
 * \param   MCAN Instance address
 *
 * \retval  status      configuration status.
 */
static int32_t App_mcanRegisterIsr();

/**
 * \brief   This is Interrupt Service Routine for MCAN interrupt 0.
 *
 * \param   none.
 *
 * \retval  none.
 */

static void App_mcan0Intr0ISR(uintptr_t arg);
static void App_mcan1Intr0ISR(uintptr_t arg);
static void App_mcan0Intr1ISR(uintptr_t arg);
static void App_mcan1Intr1ISR(uintptr_t arg);
static void App_mcanTSIntrISR(uintptr_t arg);


static int32_t App_mcanRegisterInterrupt(uint32_t intNum, void f(uintptr_t));


/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                 Internal Function Definitions                              */
/* ========================================================================== */
static int32_t App_mcanRegisterInterrupt(uint32_t intNum, void f(uintptr_t))
{
    int32_t configStatus = STW_SOK;
    OsalRegisterIntrParams_t intrPrms;
    OsalInterruptRetCode_e osalRetVal;
    HwiP_Handle hwiHandle = NULL;

    /* Enable CPU Interrupts and register ISR - MCAN0 Intr0 */
    Osal_RegisterInterrupt_initParams(&intrPrms);
    /* Populate the interrupt parameters */
    intrPrms.corepacConfig.arg = (uintptr_t)NULL;
    intrPrms.corepacConfig.isrRoutine = f;
    intrPrms.corepacConfig.priority = 0U;
    intrPrms.corepacConfig.corepacEventNum = 0U;
    intrPrms.corepacConfig.intVecNum = intNum;

    /* Register interrupts */
    osalRetVal = Osal_RegisterInterrupt(&intrPrms, &hwiHandle);
    if (OSAL_INT_SUCCESS != osalRetVal)
    {
        configStatus = CSL_EFAIL;
    }
    return configStatus;
}

static int32_t App_mcanRegisterIsr()
{
    int32_t configStatus = STW_SOK;

#if TEST_SLOT == 0
    /* MCAN0 */
    configStatus = App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST0_INT0, &App_mcan0Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST0_INT1, &App_mcan0Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST0_TS_INT, &App_mcanTSIntrISR);

    /* MCAN1 */
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST1_INT0, &App_mcan1Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST1_INT1, &App_mcan1Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST1_TS_INT, &App_mcanTSIntrISR);
#endif

#if TEST_SLOT == 1
    /* MCAN2 */
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST2_INT0, &App_mcan0Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST2_INT1, &App_mcan0Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST2_TS_INT, &App_mcanTSIntrISR);

    /* MCAN3 */
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST3_INT0, &App_mcan1Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST3_INT1, &App_mcan1Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST3_TS_INT, &App_mcanTSIntrISR);
#endif

#if TEST_SLOT == 2
    /* MCAN4 */
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST4_INT0, &App_mcan0Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST4_INT1, &App_mcan0Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST4_TS_INT, &App_mcanTSIntrISR);

    /* MCAN5 */
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST5_INT0, &App_mcan1Intr0ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST5_INT1, &App_mcan1Intr1ISR);
    configStatus += App_mcanRegisterInterrupt(APP_MAIN_MCAN_DEF_INST5_TS_INT, &App_mcanTSIntrISR);
#endif
    if (STW_SOK != configStatus)
    {
        Debug_logTag("CrossBar/Interrupt Configuration failed.\n");
    }
    else
    {
        Debug_logTag("CrossBar/Interrupt Configuration done.\n");
    }

    return configStatus;
}

static int32_t App_mcanConfig(uint32_t mcanInstAddr, bool enableInternalLpbk)
{
    uint32_t fdoe;
    int32_t configStatus = CSL_PASS;
    MCAN_RevisionId revId;
    MCAN_InitParams initParams;
    MCAN_ConfigParams configParams;
    MCAN_MsgRAMConfigParams msgRAMConfigParams;
    MCAN_StdMsgIDFilterElement stdFiltelem;
    MCAN_BitTimingParams bitTimes;

    /* Initialize MCAN Init params */
    initParams.fdMode = 0x0U;
    initParams.brsEnable = 0x0U;
    initParams.txpEnable = 0x0U;
    initParams.efbi = 0x0U;
    initParams.pxhddisable = 0x0U;
    /* To enable automatic retransmission of the packet,
     * program initParams.darEnable to "0" */
    initParams.darEnable = 0x1U;
    initParams.wkupReqEnable = 0x1U;
    initParams.autoWkupEnable = 0x1U;
    initParams.emulationEnable = 0x1U;
    initParams.emulationFAck = 0x0U;
    initParams.clkStopFAck = 0x0U;
    initParams.wdcPreload = 0xFFU;
    initParams.tdcEnable = 0x1U;
    initParams.tdcConfig.tdcf = 0xAU;
    initParams.tdcConfig.tdco = 0x6U;
    /* Initialize MCAN Config params */
    configParams.monEnable = 0x0U;
    configParams.asmEnable = 0x0U;
    configParams.tsPrescalar = 0xFU;
    configParams.tsSelect = 0x0U;
    configParams.timeoutSelect = MCAN_TIMEOUT_SELECT_CONT;
    configParams.timeoutPreload = 0xFFFFU;
    configParams.timeoutCntEnable = 0x0U;
    configParams.filterConfig.rrfs = 0x1U;
    configParams.filterConfig.rrfe = 0x1U;
    configParams.filterConfig.anfe = 0x1U;
    configParams.filterConfig.anfs = 0x1U;
    /* Initialize Message RAM Sections Configuration Parameters */
    msgRAMConfigParams.flssa = APP_MCAN_STD_ID_FILT_START_ADDR;
    msgRAMConfigParams.lss = APP_MCAN_STD_ID_FILTER_NUM;
    msgRAMConfigParams.flesa = APP_MCAN_EXT_ID_FILT_START_ADDR;
    msgRAMConfigParams.lse = APP_MCAN_EXT_ID_FILTER_NUM;
    msgRAMConfigParams.txStartAddr = APP_MCAN_TX_BUFF_START_ADDR;
    msgRAMConfigParams.txBufNum = APP_MCAN_TX_BUFF_SIZE;
    msgRAMConfigParams.txFIFOSize = 0U;
    msgRAMConfigParams.txBufMode = 0U;
    msgRAMConfigParams.txBufElemSize = MCAN_ELEM_SIZE_64BYTES;
    msgRAMConfigParams.txEventFIFOStartAddr = APP_MCAN_TX_EVENT_START_ADDR;
    msgRAMConfigParams.txEventFIFOSize = APP_MCAN_TX_BUFF_SIZE;
    msgRAMConfigParams.txEventFIFOWaterMark = 3U;
    msgRAMConfigParams.rxFIFO0startAddr = APP_MCAN_FIFO_0_START_ADDR;
    msgRAMConfigParams.rxFIFO0size = APP_MCAN_FIFO_0_NUM;
    msgRAMConfigParams.rxFIFO0waterMark = 3U;
    msgRAMConfigParams.rxFIFO0OpMode = 0U;
    msgRAMConfigParams.rxFIFO1startAddr = APP_MCAN_FIFO_1_START_ADDR;
    msgRAMConfigParams.rxFIFO1size = APP_MCAN_FIFO_1_NUM;
    msgRAMConfigParams.rxFIFO1waterMark = 3U;
    msgRAMConfigParams.rxFIFO1OpMode = 0U;
    msgRAMConfigParams.rxBufStartAddr = APP_MCAN_RX_BUFF_START_ADDR;
    msgRAMConfigParams.rxBufElemSize = MCAN_ELEM_SIZE_64BYTES;
    msgRAMConfigParams.rxFIFO0ElemSize = MCAN_ELEM_SIZE_64BYTES;
    msgRAMConfigParams.rxFIFO1ElemSize = MCAN_ELEM_SIZE_64BYTES;
    /* Initialize Tx Buffer Config params */
    stdFiltelem.sfid2 = 0x000;  // 最小标准 ID
    stdFiltelem.sfid1 = 0x7FF;  // 最大标准 ID
    stdFiltelem.sfec = 0x7;     // 匹配存入buff
    stdFiltelem.sft = 0x3;      // 00 = range filter
    /* Initialize bit timings
     * Configuring 1Mbps and 5Mbps as nominal and data bit-rate respectively */
    bitTimes.nomRatePrescalar = 0x7U;
    bitTimes.nomTimeSeg1 = 0x5U;
    bitTimes.nomTimeSeg2 = 0x2U;
    bitTimes.nomSynchJumpWidth = 0x0U;
    bitTimes.dataRatePrescalar = 0x7U;
    bitTimes.dataTimeSeg1 = 0x5U;
    bitTimes.dataTimeSeg2 = 0x2U;
    bitTimes.dataSynchJumpWidth = 0x0U;

    /* Get MCANSS Revision ID */
    MCAN_getRevisionId(mcanInstAddr, &revId);
    /* Enable Auto wakeup */
    fdoe = MCAN_isFDOpEnable(mcanInstAddr);
    if ((uint32_t)TRUE == fdoe)
    {
        Debug_logTag("CAN-FD operation is enabled through E-Fuse.\n");
    }
    else
    {
        Debug_logTag("CAN-FD operation is disabled through E-Fuse.\n");
    }
    /* wait for memory initialization to happen */
    while (FALSE == MCAN_isMemInitDone(mcanInstAddr))
    {
    }
    /* Get endianess value */
    Debug_logTag("Endianess Value: 0x%x\n", MCAN_getEndianVal(mcanInstAddr));
    /* Put MCAN in SW initialization mode */
    MCAN_setOpMode(mcanInstAddr, MCAN_OPERATION_MODE_SW_INIT);
    while (MCAN_OPERATION_MODE_SW_INIT != MCAN_getOpMode(mcanInstAddr))
    {
    }
    /* Initialize MCAN module */
    MCAN_init(mcanInstAddr, &initParams);
    /* Configure MCAN module */
    MCAN_config(mcanInstAddr, &configParams);
    /* Configure Bit timings */
    MCAN_setBitTime(mcanInstAddr, &bitTimes);
    /* Set Extended ID Mask */
    MCAN_setExtIDAndMask(mcanInstAddr, APP_MCAN_EXT_ID_AND_MASK);
    /* Configure Message RAM Sections */
    MCAN_msgRAMConfig(mcanInstAddr, &msgRAMConfigParams);
    /* Configure Standard ID filter element */
    MCAN_addStdMsgIDFilter(mcanInstAddr, 0U, &stdFiltelem);

    if (TRUE == enableInternalLpbk)
    {
        MCAN_lpbkModeEnable(mcanInstAddr, MCAN_LPBK_MODE_INTERNAL, TRUE);
    }

    /* Take MCAN out of the SW initialization mode */
    MCAN_setOpMode(mcanInstAddr, MCAN_OPERATION_MODE_NORMAL);

    while (MCAN_OPERATION_MODE_NORMAL != MCAN_getOpMode(mcanInstAddr))
    {
    }
    return configStatus;
}

static void App_mcan0Intr0ISR(uintptr_t arg)
{
    uint32_t intrStatus;

    /************** 处理 MCAN0 发送中断 **************/
    intrStatus = MCAN_getIntrStatus(gMcanTxModAddr);
    MCAN_clearIntrStatus(gMcanTxModAddr, intrStatus);

    if (MCAN_INTR_SRC_TRANS_COMPLETE == (intrStatus & MCAN_INTR_SRC_TRANS_COMPLETE))
    {
        gMcan0IsrIntr0Flag = 0U;  // CAN0 TX COMPLETE
    }
}

static void App_mcan1Intr0ISR(uintptr_t arg)
{
    uint32_t intrStatus;

    /************** 处理 MCAN1 发送中断 **************/
    intrStatus = MCAN_getIntrStatus(gMcanRxModAddr);
    MCAN_clearIntrStatus(gMcanRxModAddr, intrStatus);

    if (MCAN_INTR_SRC_TRANS_COMPLETE == (intrStatus & MCAN_INTR_SRC_TRANS_COMPLETE))
    {
        gMcan1IsrIntr0Flag = 0U;  // CAN1 TX COMPLETE
    }
}

static void App_mcan0Intr1ISR(uintptr_t arg)
{
    uint32_t intrStatus;

    /************** 处理 MCAN0 接收 **************/
    intrStatus = MCAN_getIntrStatus(gMcanTxModAddr);
    MCAN_clearIntrStatus(gMcanTxModAddr, intrStatus);

    if (MCAN_INTR_SRC_DEDICATED_RX_BUFF_MSG == (intrStatus & MCAN_INTR_SRC_DEDICATED_RX_BUFF_MSG))
    {
        gMcan0IsrIntr1Flag = 0U;  // CAN1 RX COMPLETE
    }
}

static void App_mcan1Intr1ISR(uintptr_t arg)
{
    uint32_t intrStatus;

    /************** 处理 MCAN0 接收 **************/
    intrStatus = MCAN_getIntrStatus(gMcanRxModAddr);
    MCAN_clearIntrStatus(gMcanRxModAddr, intrStatus);

    if (MCAN_INTR_SRC_DEDICATED_RX_BUFF_MSG == (intrStatus & MCAN_INTR_SRC_DEDICATED_RX_BUFF_MSG))
    {
        gMcan1IsrIntr1Flag = 0U;  // CAN1 RX COMPLETE
    }
}

static void App_mcanTSIntrISR(uintptr_t arg)
{
    Debug_logError("Time Stamp overflow happened.\n");
}

static void App_mcanPrintTxMsg(const MCAN_TxBufElement *txMsg)
{
    uint32_t loopCnt;

    Debug_logTag("Message ID: 0x%x\n", txMsg->id);
    Debug_logTag("Message Remote Transmission Request: 0x%x\n", txMsg->rtr);
    Debug_logTag("Message Extended Frame ID(0:11Bit ID/1:29bit ID): 0x%x\n", txMsg->xtd);
    Debug_logTag("Message Error State Indicator(0:Error Active/1:Error Passive): 0x%x\n",
                      txMsg->esi);
    Debug_logTag("Message Data Length Code: 0x%x\n", txMsg->dlc);
    Debug_logTag("Message BRS: 0x%x\n", txMsg->brs);
    Debug_logTag("Message CAN FD format: 0x%x\n", txMsg->fdf);
    Debug_logTag("Message Store Tx Events: 0x%x\n", txMsg->efc);
    Debug_logTag("Message Marker: 0x%x\n", txMsg->mm);

    Debug_logTag("Message DataByte: ");
    for (loopCnt = 0U; loopCnt < gMcanAppdataSize[txMsg->dlc]; loopCnt++)
    {
        Debug_log("data[%d]=0x%x ", txMsg->data[loopCnt]);
    }
    Debug_log("\n");
}

static void App_mcanPrintRxMsg(const MCAN_RxBufElement *rxMsg)
{
    uint32_t loopCnt;

    Debug_logTag("Message ID: 0x%x\n", rxMsg->id);
    Debug_logTag("Message Remote Transmission Request: 0x%x\n", rxMsg->rtr);
    Debug_logTag("Message Extended Frame ID(0:11Bit ID/1:29bit ID): 0x%x\n", rxMsg->xtd);
    Debug_logTag("Message Error State Indicator(0:Error Active/1:Error Passive): 0x%x\n",
                      rxMsg->esi);
    Debug_logTag("Message TimeStamp: 0x%x\n", rxMsg->rxts);
    Debug_logTag("Message Data Length Code: 0x%x\n", rxMsg->dlc);
    Debug_logTag("Message BRS: 0x%x\n", rxMsg->brs);
    Debug_logTag("Message CAN FD format: 0x%x\n", rxMsg->fdf);
    Debug_logTag("Message Filter Index: 0x%x\n", rxMsg->fidx);
    Debug_logTag("Message Accept Non-matching Frame: 0x%x\n", rxMsg->anmf);

    Debug_logTag("Message DataByte: ");
    for (loopCnt = 0U; loopCnt < gMcanAppdataSize[rxMsg->dlc]; loopCnt++)
    {
        Debug_log("data[%d]=0x%x ", loopCnt, rxMsg->data[loopCnt]);
    }
    Debug_log("\n");
}

static int change_mcan_dir = 0;
int mcan0_tx = 1;
void mcan_change_dir(int dir)
{
    if (dir != 0 && change_mcan_dir == 0)
    {
        if (mcan0_tx)
        {
            Debug_log("\nMCAN0 TXEN\n");
            gMcanTxModAddr = CSL_MCAN0_MSGMEM_RAM_BASE;  // CAN0
            gMcanRxModAddr = CSL_MCAN1_MSGMEM_RAM_BASE;  // CAN1
        }
        else
        {
            Debug_log("\nMCAN1 TXEN\n");
            gMcanTxModAddr = CSL_MCAN1_MSGMEM_RAM_BASE;  // CAN1
            gMcanRxModAddr = CSL_MCAN0_MSGMEM_RAM_BASE;  // CAN0
        }
        mcan_diag.recv_oks = 0;
        mcan_diag.recv_err = 0;
        mcan_set_up_lpbk(gMcanTxModAddr, gMcanRxModAddr);
        mcan0_tx = !mcan0_tx;
    }
    change_mcan_dir = dir;
}


mcan_diag_t mcan_diag = {0};
MCAN_TxBufElement gTxMsg;

void mcan_set_up_lpbk(uint32_t mcan_tx_addr, uint32_t mcan_rx_addr)
{
    int32_t configStatus = CSL_PASS;

    gMcanTxModAddr = mcan_tx_addr;  // CAN0
    gMcanRxModAddr = mcan_rx_addr;  // CAN1

    /* CrossBar Configuration */
    App_mcanRegisterIsr();

    gTxMsg.id = (uint32_t)((uint32_t)(0x7FFU) << 18U);
    gTxMsg.xtd = 0U;
    gTxMsg.rtr = 0U;    // data frame
    gTxMsg.esi = 0U;    // Error state indicator
    gTxMsg.fdf = 0U;    // standard can frame
    gTxMsg.brs = 0U;    // bit rate switch, not used in standard can frame
    gTxMsg.dlc = 0x1U;  // 1 byte data length
    gTxMsg.efc = 1U;    // Store Tx events
    gTxMsg.mm = 0xAAU;

    /* payload data */
    gTxMsg.data[0] = 1;

    MCAN_reset(gMcanTxModAddr);
    MCAN_reset(gMcanRxModAddr);
    while (MCAN_isInReset(gMcanTxModAddr) || MCAN_isInReset(gMcanRxModAddr))
    {
        /* wait for reset to complete */
    }

    configStatus += App_mcanConfig(gMcanTxModAddr, FALSE);
    configStatus += App_mcanConfig(gMcanRxModAddr, FALSE);

    if (CSL_PASS == configStatus)
    {
        /* Enable Transmission Interrupts for Tx CAN instance */
        MCAN_enableIntr(gMcanTxModAddr, MCAN_INTR_MASK_ALL, (uint32_t)TRUE);
        MCAN_enableIntr(gMcanTxModAddr, MCAN_INTR_SRC_RES_ADDR_ACCESS, (uint32_t)FALSE);

        /* Enable Transmission Interrupts for Rx CAN instance */
        MCAN_enableIntr(gMcanRxModAddr, MCAN_INTR_MASK_ALL, (uint32_t)TRUE);
        MCAN_enableIntr(gMcanRxModAddr, MCAN_INTR_SRC_RES_ADDR_ACCESS, (uint32_t)FALSE);

        /* Tx Instance Line 0 ->  App_mcan0Intr0ISR */
        MCAN_selectIntrLine(gMcanTxModAddr, MCAN_INTR_MASK_ALL, MCAN_INTR_LINE_NUM_0);

        /* Rx Instance Line 1 ->  App_mcan1Intr1ISR */
        MCAN_selectIntrLine(gMcanRxModAddr, MCAN_INTR_MASK_ALL, MCAN_INTR_LINE_NUM_1);

        MCAN_enableIntrLine(gMcanTxModAddr, MCAN_INTR_LINE_NUM_0, 1U);  // Enable Line 0 for TX CAN
        MCAN_enableIntrLine(gMcanRxModAddr, MCAN_INTR_LINE_NUM_1, 1U);  // Enable Line 1 for RX CAN

        /* Enable Transmission interrupt */
        configStatus = MCAN_txBufTransIntrEnable(gMcanTxModAddr, 1U, (uint32_t)TRUE);
        configStatus = MCAN_txBufTransIntrEnable(gMcanRxModAddr, 1U, (uint32_t)TRUE);
        if (CSL_PASS != configStatus)
        {
            Debug_logTag("Error in enabling buffer Transmit interrupt...\n");
        }
    }
    else
    {
        Debug_logTag("Configuration failed.\n");
    }

    Debug_logTag(" Test Completed.\n");
}

void mcan_ext_lpbk_test()
{
    int32_t testStatus = CSL_PASS;
    int try_cnt = 5;
    MCAN_ProtocolStatus protStatus;
    MCAN_RxBufElement rxMsg;
    MCAN_RxNewDataStatus newDataStatus;
    MCAN_ErrCntStatus errCounter;

    /* Write message to Msg RAM */
    MCAN_writeMsgRam(gMcanTxModAddr, MCAN_MEM_TYPE_BUF, 1U, &gTxMsg);
    /* Add request for transmission */
    testStatus = MCAN_txBufAddReq(gMcanTxModAddr, 1U);

    if (CSL_PASS != testStatus)
    {
        Debug_logTag("Error in Adding Transmission Request...\n");
        return;
    }

    while (gMcan0IsrIntr0Flag)
    {
        Osal_delay(10);
        try_cnt--;
        if (try_cnt <= 0)
        {
            testStatus = CSL_EFAIL;
            mcan_diag.recv_err++;
            Debug_logError("Error in Receiving Message...\n");
            return;
        }
    }
    try_cnt = 5;
    gMcan0IsrIntr0Flag = 1U;
    MCAN_getProtocolStatus(gMcanTxModAddr, &protStatus);
    /* Checking for Errors */
    if (((MCAN_ERR_CODE_NO_ERROR == protStatus.lastErrCode) ||
         (MCAN_ERR_CODE_NO_CHANGE == protStatus.lastErrCode)) &&
        ((MCAN_ERR_CODE_NO_ERROR == protStatus.dlec) ||
         (MCAN_ERR_CODE_NO_CHANGE == protStatus.dlec)) &&
        (0U == protStatus.pxe))
    {
//        Debug_logTag("Message successfully transferred with payload Bytes:%d\n",
//                          gMcanAppdataSize[gTxMsg.dlc]);
    }
    else
    {
        Debug_logTag("Error in transmission with payload Bytes:%d\n",
                          gMcanAppdataSize[gTxMsg.dlc]);
        testStatus = CSL_EFAIL;
        mcan_diag.recv_err++;
        Debug_logError("Error in transmission with payload Bytes:%d\n");
    }

    while (gMcan1IsrIntr1Flag)
    {
        Osal_delay(10);
        try_cnt--;
        if (try_cnt <= 0)
        {
            testStatus = CSL_EFAIL;
            mcan_diag.recv_err++;
            Debug_logError("Error in Receiving Message...\n");
            return;
        }
    }
    try_cnt = 5;    
    gMcan1IsrIntr1Flag = 1U;
    /* Checking for Errors */
    MCAN_getErrCounters(gMcanRxModAddr, &errCounter);

    if ((0U == errCounter.recErrCnt) && (0U == errCounter.canErrLogCnt))
    {
        MCAN_getNewDataStatus(gMcanRxModAddr, &newDataStatus);
        MCAN_clearNewDataStatus(gMcanRxModAddr, &newDataStatus);
        MCAN_readMsgRam(gMcanRxModAddr, MCAN_MEM_TYPE_BUF, 0U, 0U, &rxMsg);
        testStatus = CSL_PASS;

        if (gTxMsg.data[0] != rxMsg.data[0])
        {
            testStatus = CSL_EFAIL;
            mcan_diag.recv_err++;
        }

        if (CSL_PASS == testStatus)
        {
//             Debug_logTag("Message successfully received with payload Bytes: %d\n",
//                               gMcanAppdataSize[rxMsg.dlc]);
            mcan_diag.recv_oks++;
        }
        else
        {
            Debug_logTag("Wrong data received in message with payload Bytes: \n",
                              gMcanAppdataSize[rxMsg.dlc]);
        }
    }
    else
    {
        Debug_logTag("Error in reception with payload Bytes:%d\n",
                          gMcanAppdataSize[rxMsg.dlc]);
        testStatus = CSL_EFAIL;
    }

    if (CSL_EFAIL == testStatus)
    {
        Debug_logTag("CAN EXT Loopback Test FAILED...\n");
    }
    else
    {
//        Debug_logTag("CAN EXT Loopback Test PASSED...\n");
    }
}
