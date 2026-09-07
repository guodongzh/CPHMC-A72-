/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_intr_init.c
 *@author     LiuRui
 *@date       2025.10.09
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.10.09  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "gpio_intr_init.h"
#include <ti/csl/csl_clec.h>
#include <pscEnv.h>
#include <oem/rxfb_api.h>
#include <tfr_def.h>
#include <fpga_monitor_info.h>
#include "ft3_data.h"
#include "udma_mem_copy.h"
#include "pcie_fpga.h"
#include "ti/osal/osal_config.h"
#include "aurora_data.h"
#include "redun_switch.h"
#include "handle_int_prog.h"

#ifdef BUILD_MCU
#include "enet_queue_common.h"
#include "inet_queue_common.h"
#include "net_tftp.h"
#endif

#ifdef BUILD_C7X
#include <Hwi.h>
static HwiC7x_Struct gGpioHwiObj;
#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
extern volatile uint32_t ISR_TimeValues[4];
volatile static uint32_t pre_time = 0;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

static void clear_gpio_intr()
{
#ifndef BUILD_C66X
    GPIO_clearInt(0);
#endif
#ifdef BUILD_MCU2_0
    Osal_ClearInterrupt(0, CSLR_R5FSS0_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_25);
    Osal_EnableInterrupt(0, CSLR_R5FSS0_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_25);
#elif defined(BUILD_MCU2_1)
    Osal_ClearInterrupt(0, CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_29);
    Osal_EnableInterrupt(0, CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_29);
#elif defined(BUILD_MCU3_0)
    Osal_ClearInterrupt(0, CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_17);
    Osal_EnableInterrupt(0, CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_17);
#elif defined(BUILD_MCU3_1)
    Osal_ClearInterrupt(0, CSLR_R5FSS1_CORE1_INTR_GPIOMUX_INTRTR0_OUTP_21);
    Osal_EnableInterrupt(0, CSLR_R5FSS1_CORE1_INTR_GPIOMUX_INTRTR0_OUTP_21);
#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C66X_2)

#elif defined(BUILD_C7X_1)

    Hwi_clearInterrupt(1417);
    Hwi_enableInterrupt(1417);
#elif defined(BUILD_MCU1_1)
    Osal_ClearInterrupt(0, CSLR_MCU_R5FSS0_CORE1_INTR_MAIN2MCU_PLS_INTRTR0_OUTP_25);
    Osal_EnableInterrupt(0, CSLR_MCU_R5FSS0_CORE1_INTR_MAIN2MCU_PLS_INTRTR0_OUTP_25);
#endif
}

static void intr_recv_handler(void)
{
    for (int i = 0; i < sizeof(core_train_rx) / sizeof(core_train_rx[0]); ++i)
    {
        if (core_train_rx[i].train_rx == NULL)
        {
            break;
        }
#ifdef BUILD_C66X
        udma_memcpy(core_train_c66rx[i].train_rx->header,
                    core_train_rx[i].train_rx->header,
                    TRAIN_HEADER_LEN);
        udma_memcpy_wait_complete(0xffffffff);
        CacheP_Inv(core_train_c66rx[i].train_rx->header, TRAIN_HEADER_LEN);

        /*get base addr*/
        car_rx_t **car_src = NULL, **car_dest = NULL;
        if (core_train_c66rx[i].train_rx->header->car_id == 1)
        {
            car_dest = core_train_c66rx[i].train_rx->car0;
            car_src = core_train_rx[i].train_rx->car0;
        }
        else
        {
            car_dest = core_train_c66rx[i].train_rx->car1;
            car_src = core_train_rx[i].train_rx->car1;
        }

        for (int j = 0; j < core_train_c66rx[i].train_rx->header->car_num; ++j)
        {
            udma_memcpy(car_dest[j], car_src[j], core_train_c66rx[i].train_rx->car_size);
        }
        udma_memcpy_wait_complete(0xffffffff);

        train_recv(core_train_c66rx[i].train_rx, core_train_c66rx[i].slot_num);
#else
        train_recv(core_train_rx[i].train_rx, core_train_rx[i].slot_num);
#endif
    }

#ifdef BUILD_C66X
    udma_memcpy(&g_shm_irigb_info_c6x, &g_shm_irigb_info, sizeof(CLK_TIME_EDGE));
    PscGetWatchDataStart();
#endif

    for (int i = 0; i < sizeof(core_train_tx_ft3) / sizeof(core_train_tx_ft3[0]); ++i)
    {
        if (core_train_tx_ft3[i].train_tx == NULL)
        {
            break;
        }
        ft3_build_tx_car(core_train_tx_ft3[i].train_tx,
                         core_train_tx_ft3[i].slot_num,
                         &pcie_ft3_data);
    }

    for (int i = 0; i < sizeof(core_train_tx_aurora) / sizeof(core_train_tx_aurora[0]); ++i)
    {
        if (core_train_tx_aurora[i].train_tx == NULL)
        {
            break;
        }
        aurora_build_tx_car(core_train_tx_aurora[i].train_tx,
                            core_train_tx_aurora[i].slot_num,
                            &pcie_aurora_data);
    }

#ifdef BUILD_MCU2_0
    inet_fpga_to_rxfifo();
    inet_recv_to_rxfifo();
    ipc_scan_all_core_enet_txque_to_pcie_txque();
    push_enet_frm_to_pcie();
    fpga_monitor_info_update(slot0_1_train_rx[3].header);
    fpga_monitor_info_update(slot2_train_rx[3].header);
    fpga_monitor_info_update(slot3_train_rx[3].header);
#elif defined(BUILD_MCU2_1)
    inet_fpga_to_rxfifo();
    inet_recv_to_rxfifo();
    send_to_ethernet();
#elif defined(BUILD_MCU3_0)
    inet_recv_to_rxfifo();
    send_to_ethernet();
#elif defined(BUILD_MCU3_1)
    inet_recv_to_rxfifo();
    send_to_ethernet();
    reset_ft3_diag_processing_flags();
#elif defined(BUILD_C66X_1)
    reset_ft3_diag_processing_flags();
    reset_aurora_diag_processing_flags();
#elif defined(BUILD_C66X_2)
    reset_ft3_diag_processing_flags();
    reset_aurora_diag_processing_flags();
#elif defined(BUILD_C7X_1)
    reset_ft3_diag_processing_flags();
//    reset_aurora_diag_processing_flags();
#elif defined(BUILD_MCU1_1)
    reset_redun_switch_diag_processing_flags();
    redun_switch_build_tx_car();
#endif
}

static void intr_other_handler(void)
{
    for (int i = 0; i < sizeof(core_train_tx_ft3) / sizeof(core_train_tx_ft3[0]); ++i)
    {
        if (core_train_tx_ft3[i].train_tx == NULL)
        {
            break;
        }
        ft3_calc_checksum(&pcie_ft3_data, core_train_tx_ft3[i].slot_num);
    }
    for (int i = 0; i < sizeof(core_train_tx_aurora) / sizeof(core_train_tx_aurora[0]); ++i)
    {
        if (core_train_tx_aurora[i].train_tx == NULL)
        {
            break;
        }
        aurora_calc_checksum(&pcie_aurora_data, core_train_tx_aurora[i].slot_num);
    }

#ifdef BUILD_MCU2_0

#elif defined(BUILD_MCU2_1)

#elif defined(BUILD_MCU3_0)

#elif defined(BUILD_MCU3_1)

#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C66X_2)

#elif defined(BUILD_C7X_1)

#elif defined(BUILD_MCU1_1)
    redun_switch_calc_checksum();
#endif
}

static void intr_send_handler(void)
{
#ifdef BUILD_MCU2_0

#elif defined(BUILD_MCU2_1)

#elif defined(BUILD_MCU3_0)

#elif defined(BUILD_MCU3_1)

#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C66X_2)

#elif defined(BUILD_C7X_1)

#endif
    for (int i = 0; i < sizeof(core_train_tx) / sizeof(core_train_tx[0]); ++i)
    {
        if (core_train_tx[i] == NULL)
        {
            break;
        }
#ifdef BUILD_C66X
        train_tx_update_header(core_train_c66tx[i]);
        uint32_t length = TRAIN_HEADER_LEN;
        if (core_train_c66tx[i]->car_size_s != 0)
        {
            length += (core_train_c66tx[i]->car_size_s * core_train_c66tx[i]->car_num_s * 2);
        }
        else
        {
            length += (core_train_c66tx[i]->car_size_b * core_train_c66tx[i]->car_num_b * 2);
        }

        udma_memcpy((uint8_t *)core_train_tx[i]->header,
                    (uint8_t *)core_train_c66tx[i]->header,
                    length);
        udma_memcpy_wait_complete(0xffffffff);
#else
        train_tx_update_header(core_train_tx[i]);
#endif
    }
}

static void main_soe_upload(void)
{
#ifdef BUILD_MCU
#include "ipc_scada_rpmsg.h"
    static uint32_t soe_counter = 0;
#ifdef BUILD_MCU2_0
    uint32_t swi_time_interval = init_param.core0_swi_time_interval;
#elif defined(BUILD_MCU2_1)
    uint32_t swi_time_interval = init_param.core1_swi_time_interval;
#elif defined(BUILD_MCU3_0)
    uint32_t swi_time_interval = init_param.core2_swi_time_interval;
#elif defined(BUILD_MCU3_1)
    uint32_t swi_time_interval = init_param.core3_swi_time_interval;
#elif defined(BUILD_MCU1_1)
    uint32_t swi_time_interval = init_param.core7_swi_time_interval;
#endif
    if (++soe_counter >= swi_time_interval)
    {
        soe_counter = 0;
        soe_upload();  // 1ms
    }

#endif
}

static void main_hwi_post_swi(void)
{
    static uint32_t isr_counter = 0;
#ifdef BUILD_MCU2_0
    uint32_t swi_time_interval = init_param.core0_swi_time_interval;
#elif defined(BUILD_MCU2_1)
    uint32_t swi_time_interval = init_param.core1_swi_time_interval;
#elif defined(BUILD_MCU3_0)
    uint32_t swi_time_interval = init_param.core2_swi_time_interval;
#elif defined(BUILD_MCU3_1)
    uint32_t swi_time_interval = init_param.core3_swi_time_interval;
#elif defined(BUILD_C66X_1)
    uint32_t swi_time_interval = init_param.core4_swi_time_interval;
#elif defined(BUILD_C66X_2)
    uint32_t swi_time_interval = init_param.core5_swi_time_interval;
#elif defined(BUILD_C7X)
    uint32_t swi_time_interval = init_param.core6_swi_time_interval;
#elif defined(BUILD_MCU1_1)
    uint32_t swi_time_interval = init_param.core7_swi_time_interval;
#endif
    if (++isr_counter >= swi_time_interval)
    {
        isr_counter = 0;
        handle_soft_int_prog();
    }
}

void gpio_call_back(uintptr_t arg)
{
    uint32_t time0 = 0;
    uint32_t time1 = 0;
    uint32_t time2 = 0;
    uint32_t time3 = 0;
    TimeStamp_Struct tStamp;
#if defined(BUILD_MCU) || defined(BUILD_C7X)
    float time_ratio = 0.00100;
#elif defined(BUILD_C66X)
    float time_ratio = 0.00074;
#endif
    osalArch_TimestampGet64_2(&tStamp);
    time0 = tStamp.lo;
    if (time0 > pre_time)
    {
        ISR_TimeValues[0] = (time0 - pre_time) * time_ratio;
    }
    pre_time = time0;

    main_hwi_post_swi();

    /*recv data*/
    intr_recv_handler();

    /*pscode program*/
    {
#if !defined(BUILD_MCU2_0) && !defined(BUILD_MCU2_1)
        fb_UpdateSysTick();
#ifndef BUILD_C66X
        PscSetVariable();
#endif

        osalArch_TimestampGet64_2(&tStamp);
        time1 = tStamp.lo;
        Application_I1_FI();
        Application_I1_N();
        Application_I1_FE();
        osalArch_TimestampGet64_2(&tStamp);
        time2 = tStamp.lo;

#if defined(BUILD_C66X) || defined(BUILD_C7X_1)
        PscGetWatchData();
#endif
#if defined(BUILD_C66X)
        PscGetWaveData_c6x();
#else
        PscGetWaveData();
#endif
#elif defined(BUILD_MCU2_0)
        PscWaveTmpTransfer();
#endif
    }

    handle_hard_int_prog();

    /*send data*/
    intr_other_handler();
    intr_send_handler();

#ifdef BUILD_C66X
#include "ipc_fast_data.h"
    ipc_wr_update();
#ifdef BUILD_C66X_1
    ipc_rd_update(2);
    ipc_rd_update(3);
    ipc_rd_update(5);
    ipc_rd_update(6);
#else
    ipc_rd_update(2);
    ipc_rd_update(3);
    ipc_rd_update(4);
    ipc_rd_update(6);
#endif
#endif

    osalArch_TimestampGet64_2(&tStamp);
    time3 = tStamp.lo;

    if (time2 > time1)
    {
        ISR_TimeValues[1] = (time2 - time1) * time_ratio;
    }

    if (time3 > time0)
    {
        ISR_TimeValues[2] = (time3 - time0) * time_ratio;
    }

    if (ISR_TimeValues[2] > ISR_TimeValues[3])
    {
       ISR_TimeValues[3] = ISR_TimeValues[2];
    }
    clear_gpio_intr();
}

/**
 * @brief gpio interupt test
 */

void gpio_intr_init(void)
{
#ifndef BUILD_C66X
    Debug_log("\n");
    GPIO_log("setup gpio intr...\n");

#if defined(BUILD_MCU)
    /* RegisterInterrupt */
    OsalRegisterIntrParams_t interruptRegParams;
    HwiP_Handle *hwiHandle = NULL;

    /* Initialize with defaults */
    Osal_RegisterInterrupt_initParams(&interruptRegParams);
#endif

    /* Host Interrupt vector */
#if defined(BUILD_MCU2_0)
    interruptRegParams.corepacConfig.intVecNum =
        (int32_t)CSLR_R5FSS0_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_25;
#elif defined(BUILD_MCU2_1)
    interruptRegParams.corepacConfig.intVecNum =
        (int32_t)CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_29;
#elif defined(BUILD_MCU3_0)
    interruptRegParams.corepacConfig.intVecNum =
        (int32_t)CSLR_R5FSS1_CORE0_INTR_GPIOMUX_INTRTR0_OUTP_17;
#elif defined(BUILD_MCU3_1)
    interruptRegParams.corepacConfig.intVecNum =
        (int32_t)CSLR_R5FSS1_CORE1_INTR_GPIOMUX_INTRTR0_OUTP_21;
#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C66X_1)

#elif defined(BUILD_C7X_1)

#elif defined(BUILD_MCU1_1)
    interruptRegParams.corepacConfig.intVecNum =
        (int32_t)CSLR_MCU_R5FSS0_CORE1_INTR_MAIN2MCU_PLS_INTRTR0_OUTP_25;

#endif

#if defined(BUILD_MCU)

    interruptRegParams.corepacConfig.isrRoutine = gpio_call_back;
    interruptRegParams.corepacConfig.arg = 0;
    interruptRegParams.corepacConfig.corepacEventNum = 0; /*not use in r5 core */
    interruptRegParams.corepacConfig.priority = 8;
    interruptRegParams.corepacConfig.triggerSensitivity = OSAL_ARM_GIC_TRIG_TYPE_EDGE;
    Osal_RegisterInterrupt(&interruptRegParams, hwiHandle);

    /* Enable specific gpio pin interrupt */
    GPIO_enableInt(GPIO_INT_IDX);

#if defined(BUILD_MCU2_0)

    GPIO_enableInt(GPIO_INT_IDX_C66_0);
    GPIO_enableInt(GPIO_INT_IDX_C66_1);

#endif

#elif defined(BUILD_C7X)

    /* Configure SOC interrupt path if any */
    CSL_ClecEventConfig cfgClec;
    CSL_CLEC_EVTRegs *clecBaseAddr = (CSL_CLEC_EVTRegs *)CSL_COMPUTE_CLUSTER0_CLEC_REGS_BASE;

    /* Configure CLEC for GPIO */
    cfgClec.secureClaimEnable = FALSE;
    cfgClec.evtSendEnable = TRUE;
    cfgClec.rtMap = CSL_clecGetC7xRtmapCpuId();
    cfgClec.extEvtNum = 0;
    cfgClec.c7xEvtNum = 1; /* CPU Interrupt vector */

    uint32_t evtNum = CSLR_COMPUTE_CLUSTER0_CLEC_SOC_EVENTS_IN_GPIOMUX_INTRTR0_OUTP_41 + C7X_CLEC_OFFSET;
    CSL_clecConfigEventLevel(clecBaseAddr, evtNum, 0);
    CSL_clecConfigEvent(clecBaseAddr, evtNum, &cfgClec);

    /* Register interrupts */

    /* Register interrupt directly via CSL Hwi (baremetal), bypass OSAL */
     Hwi_Params hwiParams;
     Hwi_Params_init(&hwiParams);
     hwiParams.arg         = 0;
     hwiParams.eventId     = 0;      /* ԭ corepacEventNum */
     hwiParams.priority    = 1;      /* ԭ priority 0x1 */
     hwiParams.enableInt   = 1;
     hwiParams.maskSetting = Hwi_MaskingOption_SELF;

     int32_t iStat = Hwi_construct(&gGpioHwiObj, 1 /* CPU �ж�����,ԭ intVecNum */,
                                   (Hwi_FuncPtr)gpio_call_back, &hwiParams);
     DebugP_assertNoLog(iStat == 0);

    /* Enable GPIO interrupt on the specific gpio pin */
    GPIO_enableInt(GPIO_INT_IDX);
#endif

#else
    HwiP_Params hwiParams;
    HwiP_Handle hHwi;

    /**
     * map event INTROUTER0_OUTL_61(gpio inter output) to vectid 8
     */
    HwiP_Params_init(&hwiParams);
    hwiParams.evtId = CSLR_C66SS0_CORE0_C66_EVENT_IN_SYNC_C66SS0_INTROUTER0_OUTL_61;
    hwiParams.priority = 1;
    hHwi = HwiP_create(CSL_INTC_VECTID_8, gpio_call_back, &hwiParams);
    DebugP_assertNoLog(hHwi != NULL);

#endif
}
