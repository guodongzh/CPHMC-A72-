/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       spi_cfg.c
 *@author     LiuRui
 *@date       2026.01.26
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.01.26  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "udma_mem_copy.h"
#include "spi.h"
#include "gpio_ctrl.h"
#include "pcie_spi_cfg.h"
#include "ft3_data.h"
#include "squ_mod.h"
#include "bsp_init.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
/* UDMA Ring Memory Pointer */
/* ring setting */
#define RING_ELEMENT_CNT  (1U)
#define RING_ELEMENT_SIZE (8U) /* 8 bytes */

#define SPI_USE_DMA 0

uint8_t gSpiCfgTxFqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgTxCqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgTxTdRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgTxPdMem[RING_ELEMENT_CNT][UDMA_TRPD_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

uint8_t gSpiCfgRxFqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgRxCqRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgTdRingMem[RING_ELEMENT_CNT][RING_ELEMENT_SIZE]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint8_t gSpiCfgRxPdMem[RING_ELEMENT_CNT][UDMA_TRPD_SIZE_ALIGN]
    __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

/**
 * UDMA driver objects
 */
struct Udma_ChObj gSpiCfgUdmaTxChObj;
struct Udma_ChObj gSpiCfgUdmaRxChObj;

/*APP*/
#define BUF_SIZE                  (256u)
#define FIFO_TRIGGER_LEVEL        (8u)
#define Master_MCSPI_BASE_ADDRESS CSL_MCU_MCSPI0_CFG_BASE
#define Master_MCSPI_CH           MCSPI_CHANNEL_2
uint16_t gCfgBuffer[BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));
uint16_t gCfgBakBuffer[BUF_SIZE] __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT)));

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
static void spi_dma_transmit(uint16_t *tx_buf, uint16_t *rx_buf, uint32_t data_num)
{
    int32_t retVal = UDMA_SOK;

    /* SPIEN/SPICS line is forced to low state.*/
    McSPICSAssert(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
    Osal_delay(1);
    CSL_UdmapCppi5HMPD *pHpdMemRx = (CSL_UdmapCppi5HMPD *)&gSpiCfgTxPdMem[0][0];
    CSL_UdmapCppi5HMPD *pHpdMemTx = (CSL_UdmapCppi5HMPD *)&gSpiCfgTxPdMem[0][0];

    UDMA_Hpd_Init(&gSpiCfgUdmaRxChObj, pHpdMemRx, Udma_appVirtToPhyFxn(rx_buf), data_num * 2);
    /*push to rx queue*/
    retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gSpiCfgUdmaRxChObj),
                               Udma_appVirtToPhyFxn(pHpdMemRx));
    if (retVal != UDMA_SOK)
    {
        SPI_log("%s:Failed to submit RX CQ code: %d \n", __func__, retVal);
    }

    UDMA_Hpd_Init(&gSpiCfgUdmaTxChObj, pHpdMemTx, Udma_appVirtToPhyFxn(tx_buf), data_num * 2);
    /*push to tx queue*/
    retVal = Udma_ringQueueRaw(Udma_chGetFqRingHandle(&gSpiCfgUdmaTxChObj),
                               Udma_appVirtToPhyFxn(pHpdMemTx));
    if (retVal != UDMA_SOK)
    {
        SPI_log("%s:Failed to submit TX CQ code: %d \n", __func__, retVal);
    }

    uint32_t cq_occ, time_out_count = 5;
    while (1)
    {
        cq_occ = Udma_ringGetForwardRingOcc(Udma_chGetCqRingHandle(&gSpiCfgUdmaRxChObj));
        if (cq_occ != 0)
        {
            retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gSpiCfgUdmaRxChObj),
                                         (uint64_t *)Udma_appVirtToPhyFxn(pHpdMemRx));
            if (UDMA_SOK != retVal)
            {
                DMA_log("%s:Udma_ringDequeueRaw fail %d\r\n", __func__, retVal);
                DMA_log("\r\n");
            }
            retVal = Udma_ringDequeueRaw(Udma_chGetCqRingHandle(&gSpiCfgUdmaTxChObj),
                                         (uint64_t *)Udma_appVirtToPhyFxn(pHpdMemTx));
            if (UDMA_SOK != retVal)
            {
                DMA_log("%s:Udma_ringDequeueRaw fail %d\r\n", __func__, retVal);
                DMA_log("\r\n");
            }
            break;
        }
        Osal_delay(1);
        if (--time_out_count == 0)
        {
            DMA_log("[Error] Time out!!\n");
            retVal = UDMA_EFAIL;
            break;
        }
    }

    /* Force SPIEN/SPICS line to the inactive state.*/
    McSPICSDeAssert(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
}

static void spi_polled_transmit(uint16_t *tx_buf, uint16_t *rx_buf, uint32_t data_num)
{
    uint32_t channelStatus;
    volatile uint32_t timeout1;
    /* SPIEN/SPICS line is forced to low state.*/
    McSPICSAssert(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
    Osal_delay(1);
    for (int i = 0; i < data_num; ++i)
    {
        /*master send data*/
        McSPITransmitData(Master_MCSPI_BASE_ADDRESS, (uint32_t)(tx_buf[i]), Master_MCSPI_CH);

        /*master receive data*/
        timeout1 = 0xF;
        channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
        while (MCSPI_CH0STAT_RXS_EMPTY == (channelStatus & MCSPI_CH0STAT_RXS_MASK))
        {
            channelStatus = McSPIChannelStatusGet(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
            --timeout1;
            Osal_delay(1);
            if (0 == timeout1)
            {
                SPI_log("\nMCU Master_MCSPI RX Timed out!!");
            }
        }
        rx_buf[i] = (uint16_t)McSPIReceiveData(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
    }
    /* Force SPIEN/SPICS line to the inactive state.*/
    McSPICSDeAssert(Master_MCSPI_BASE_ADDRESS, Master_MCSPI_CH);
}

/**
 * @brief
 * @param cfg
 * @param cfg_bak
 * @param payload_len payload length (in byte)
 * @return
 */
static int pcie_send_cfg_by_spi(uint16_t *cfg, uint16_t *cfg_bak, uint32_t payload_len)
{
    uint16_t offset = 0;

    /* calc sum check */
    uint16_t sum_check = 0, *sum_check_ptr;
    offset = sizeof(pcie_cfg_head_spi_t) / 2;
    sum_check_ptr = (&cfg[offset]);
    for (int i = 0; i < payload_len / 2; ++i)
    {
        sum_check += sum_check_ptr[i];
    }
    offset = (sizeof(pcie_cfg_head_spi_t) + payload_len) / 2;
    cfg[offset] = sum_check;

    /* Transfer data include sum_check*/
    offset = offset + 1;
#if SPI_USE_DMA == 1
    spi_dma_transmit(cfg, cfg_bak, offset);
#else
    spi_polled_transmit(cfg, cfg_bak, offset);
#endif
    Osal_delay(1);
#if SPI_USE_DMA == 1
    spi_dma_transmit(cfg, cfg_bak, offset);
#else
    spi_polled_transmit(cfg, cfg_bak, offset);
#endif

    if (*cfg_bak != FPGA_TO_CPU)
    {
        Debug_logError("header is err!!\n");
        return -1;
    }

    if (((pcie_cfg_head_spi_t *)cfg)->cfg_type == CFG_TYPE_INIT)
    {
        // 如果是初始化配置，只需要检查前12个字节
        if (memcmp(&cfg[sizeof(pcie_cfg_head_spi_t)/2],
                   &cfg_bak[sizeof(pcie_cfg_head_spi_t)/2],
                   12) != 0)
        {
            Debug_logError("cfg data is mismatch!!\n");
            return -1;
        }
    }
    else
    {
        if (memcmp(&cfg[sizeof(pcie_cfg_head_spi_t)/2],
                   &cfg_bak[sizeof(pcie_cfg_head_spi_t)/2],
                   payload_len) != 0)
        {
            Debug_logError("cfg data is mismatch!!\n");
            return -1;
        }
    }

    uint16_t sum_check_bak = 0;
    offset = sizeof(pcie_cfg_head_spi_t) / 2;
    sum_check_ptr = &cfg_bak[offset];
    for (int i = 0; i < payload_len / 2; ++i)
    {
        sum_check_bak += sum_check_ptr[i];
    }

    offset = (sizeof(pcie_cfg_head_spi_t) + payload_len) / 2;
    if (((pcie_cfg_head_spi_t *)cfg)->cfg_type == CFG_TYPE_INIT)
    {
        if (sum_check_bak == cfg_bak[offset])
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (sum_check_bak == sum_check && sum_check_bak == cfg_bak[offset])
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
}

static int train_irq_tim_cfg_spi(spi_sys_config_t *sys_config,
                          uint16_t t1_us,
                          uint16_t t2_us,
                          uint16_t t3_us,
                          uint16_t t4_us,
                          uint16_t t5_us,
                          uint16_t t6_us,
                          uint16_t t7_us,
                          uint16_t t8_us)
{
    if (sys_config == NULL)
        return -1;

    sys_config->ratio_irq.ratio_irq1 = t1_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq2 = t2_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq3 = t3_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq4 = t4_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq5 = t5_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq6 = t6_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq7 = t7_us / (ONLY_1S_TIME / AD_SAMPRATE);
    sys_config->ratio_irq.ratio_irq8 = t8_us / (ONLY_1S_TIME / AD_SAMPRATE);
    return 0;
}

static int train_tx_irq_num_cfg_spi(spi_sys_config_t *sys_config,
                             uint8_t exp_irq,
                             uint8_t fast1_irq,
                             uint8_t fast2_irq,
                             uint8_t fast3_irq,
                             uint8_t fast4_irq,
                             uint8_t fast5_irq,
                             uint8_t fast6_irq,
                             uint8_t fast7_irq,
                             uint8_t fast8_irq,
                             uint8_t slow_irq)
{
    if (sys_config == NULL)
        return -1;

    sys_config->tx_car[0].irq_num = exp_irq;
    sys_config->tx_car[1].irq_num = fast1_irq;
    sys_config->tx_car[2].irq_num = fast2_irq;
    sys_config->tx_car[3].irq_num = fast3_irq;
    sys_config->tx_car[4].irq_num = fast4_irq;
    sys_config->tx_car[5].irq_num = fast5_irq;
    sys_config->tx_car[6].irq_num = fast6_irq;
    sys_config->tx_car[7].irq_num = fast7_irq;
    sys_config->tx_car[8].irq_num = fast8_irq;
    sys_config->tx_car[9].irq_num = slow_irq;

    return 0;
}

static int train_rx_irq_num_cfg_spi(spi_sys_config_t *sys_config,
                             uint8_t exp_irq,
                             uint8_t fast_irq_s,
                             uint8_t fast_irq_b,
                             uint8_t slow_irq)
{
    if (sys_config == NULL)
        return -1;

    sys_config->rx_car[0].irq_num = exp_irq;
    sys_config->rx_car[1].irq_num = fast_irq_s;
    sys_config->rx_car[2].irq_num = fast_irq_b;
    sys_config->rx_car[3].irq_num = slow_irq;

    return 0;
}

static void pcie_send_ft3_cfg_spi(uint32_t slot_id)
{
    int32_t retVal;
    uint16_t cfg_data_offset = sizeof(pcie_cfg_head_spi_t) / 2;
    pcie_cfg_head_spi_t *spi_pcie_cfg = (pcie_cfg_head_spi_t *)gCfgBuffer;

    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_FT3;
    spi_pcie_cfg->cfg_slot_id = slot_id;
    spi_pcie_cfg->cfg_payload_len = sizeof(ft3_cfg_t) / 2;

    ft3_cfg_t *ft3_cfg_spi = (ft3_cfg_t *)(&gCfgBuffer[cfg_data_offset]);
    for (int ch_num = 0; ch_num < FT3_TX_CH_NUM; ++ch_num)
    {
        /*Is the parameter valid?*/
        if (ft3_tx_cfg_all[slot_id][ch_num].cfg_state == CFG_VALID)
        {
            spi_pcie_cfg->cfg_port_id = ch_num;
            memcpy(ft3_cfg_spi,
                   &ft3_tx_cfg_all[slot_id][ch_num].cfg_data,
                   sizeof(ft3_cfg_t));
            retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(ft3_cfg_t));
            if (retVal == 0)
            {
                ft3_tx_cfg_all[slot_id][ch_num].cfg_state = CFG_PASS;
                memcpy(&ft3_tx_cfg_all[slot_id][ch_num].cfg_data_bak,
                       &gCfgBakBuffer[cfg_data_offset],
                       sizeof(ft3_cfg_t));
                Debug_logOk(" SLOT[%d]-CH:[%d] ft3-tx cfg OK!!\n", slot_id, ch_num);
            }
            else
            {
                ft3_tx_cfg_all[slot_id][ch_num].cfg_state = CFG_FAIL;
                Debug_logError(" SLOT[%d]-CH:[%d] ft3-tx cfg failed!!\n", slot_id, ch_num);
            }
        }
    }

    for (int ch_num = 0; ch_num < FT3_RX_CH_NUM; ++ch_num)
    {
        /*Is the parameter valid?*/
        if (ft3_rx_cfg_all[slot_id][ch_num].cfg_state == CFG_VALID)
        {
            spi_pcie_cfg->cfg_port_id = ch_num;
            memcpy(ft3_cfg_spi,
                   &ft3_rx_cfg_all[slot_id][ch_num].cfg_data,
                   sizeof(ft3_cfg_t));
            retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(ft3_cfg_t));
            if (retVal == 0)
            {
                ft3_rx_cfg_all[slot_id][ch_num].cfg_state = CFG_PASS;
                memcpy(&ft3_rx_cfg_all[slot_id][ch_num].cfg_data_bak,
                       &gCfgBakBuffer[cfg_data_offset],
                       sizeof(ft3_cfg_t));
                Debug_logOk(" SLOT[%d]-CH:[%d] ft3-rx cfg OK!!\n", slot_id, ch_num);
            }
            else
            {
                ft3_rx_cfg_all[slot_id][ch_num].cfg_state = CFG_FAIL;
                Debug_logError(" SLOT[%d]-CH:[%d] ft3-rx cfg failed!!\n", slot_id, ch_num);
            }
        }
    }
    CacheP_wb(ft3_tx_cfg_all, sizeof(ft3_tx_cfg_all));
    CacheP_wb(ft3_rx_cfg_all, sizeof(ft3_rx_cfg_all));
}

void spi_config_init()
{
    SPI_Params_t masterParam;
    int32_t retVal = UDMA_SOK;
    SPI_DmaParams_t dmaParams;

    /* config master SPI */
    masterParam.baseAddr = Master_MCSPI_BASE_ADDRESS;
    masterParam.chNum = Master_MCSPI_CH;
    masterParam.afl = FIFO_TRIGGER_LEVEL;
    masterParam.ael = FIFO_TRIGGER_LEVEL;
    masterParam.mode = TX_RX_MODE;
    masterParam.isMaster = true;
    masterParam.frame = SPI_POL0_PHA0_;
    masterParam.wordLen = 16;
#if SPI_USE_DMA == 1
    masterParam.dmaEnable = true;
#else
    masterParam.dmaEnable = false;
#endif

    spi_init(masterParam);

    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    CacheP_wbInv((void *)&gCfgBuffer[0], sizeof(gCfgBuffer));
    CacheP_wbInv((void *)&gCfgBakBuffer[0], sizeof(gCfgBakBuffer));

#if SPI_USE_DMA == 1
    /* config tx udma */
    dmaParams.drvHandle = &gUdmaDrvObj;
    dmaParams.chHandle = &gSpiCfgUdmaTxChObj;
    dmaParams.fqRingMem = gSpiCfgTxFqRingMem;
    dmaParams.cqRingMem = gSpiCfgTxCqRingMem;
    dmaParams.tdRingMem = gSpiCfgTxTdRingMem;
    dmaParams.elemCnt = RING_ELEMENT_CNT;
    dmaParams.peerChNum = CSL_PDMA_CH_MCU_MCSPI0_CH0_TX;
    retVal = spi_tx_udma_config(&dmaParams);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA config failed\r\n", __func__);
    }

    /* config rx udma */
    dmaParams.drvHandle = &gUdmaDrvObj;
    dmaParams.chHandle = &gSpiCfgUdmaRxChObj;
    dmaParams.fqRingMem = gSpiCfgRxFqRingMem;
    dmaParams.cqRingMem = gSpiCfgRxCqRingMem;
    dmaParams.tdRingMem = gSpiCfgTdRingMem;
    dmaParams.elemCnt = RING_ELEMENT_CNT;
    dmaParams.peerChNum = CSL_PDMA_CH_MCU_MCSPI0_CH0_RX;
    retVal = spi_rx_udma_config(&dmaParams);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA config failed\r\n", __func__);
    }

    /* config tx pdma */
    retVal = spi_tx_pdma_config(&gSpiCfgUdmaTxChObj, masterParam.afl, 512, masterParam.wordLen);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA TX channel config failed\r\n", __func__);
    }
    /* config rx pdma */
    retVal = spi_rx_pdma_config(&gSpiCfgUdmaRxChObj, masterParam.ael, 512, masterParam.wordLen);
    if (UDMA_SOK != retVal)
    {
        SPI_log("%s: UDMA RX channel config failed\r\n", __func__);
    }
#endif
}

void slot0_1_send_cfg_spi()
{
    int32_t retVal;
    uint16_t cfg_data_offset = 0;
    cfg_data_offset = sizeof(pcie_cfg_head_spi_t) / 2;
    pcie_cfg_head_spi_t *spi_pcie_cfg = (pcie_cfg_head_spi_t *)gCfgBuffer;

    Debug_log("\n");
    Debug_logTag("sending slot0 and slot1 pcie cfg...\n");

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x0;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init_t *spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_t *spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_CFG;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));
    if (retVal != 0)
    {
        Debug_logError(" slot0 and slot1 start send cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot0 and slot1 start send cfg OK!!\n");
    }

    /* systeam cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_SYS;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x0;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_config_t) / 2;

    spi_sys_config_t *sys_config = (spi_sys_config_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_config_t *sys_config_bak = (spi_sys_config_t *)(&gCfgBakBuffer[cfg_data_offset]);
    train_irq_tim_cfg_spi(sys_config,
                          init_param.core0_main_intr_time,
                          init_param.core1_main_intr_time,
                          init_param.core2_main_intr_time,
                          init_param.core3_main_intr_time,
                          init_param.core4_main_intr_time,
                          init_param.core5_main_intr_time,
                          init_param.core6_main_intr_time,
                          init_param.core7_main_intr_time);
    train_tx_irq_num_cfg_spi(sys_config, 7, 7, 8, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg_spi(sys_config, 7, 8, 7, 1);
    sys_config->slot_id = 0;
    sys_config->ad_samp_ratio = 100000;
    sys_config->soft_err_en = 0;
    sys_config->vcc_fp_en = 0;
    pcie_set_squ_mod_cfg_2_syscfg(sys_config);

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_config_t));
    pcie_get_syscfg_2_squ_mod_cfg(sys_config_bak, retVal == 0);
    if (retVal != 0)
    {
        Debug_logError(" slot0 and slot1 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot0 and slot1 send sys cfg OK!!\n");
    }

    /* geth cfg*/
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_GETH;
    spi_pcie_cfg->cfg_slot_id = 1;
    spi_pcie_cfg->cfg_payload_len = sizeof(geth_cfg_spi_t) / 2;

    geth_cfg_spi_t *spi_geth_cfg = (geth_cfg_spi_t *)(&gCfgBuffer[cfg_data_offset]);

    /**
     * sfp port 0~1：为冗余通信接口，CPU不需要配置
     * sfp port 2: 阀控通信接口 50us
     */
    spi_geth_cfg->train_rx_id = 3;
    spi_geth_cfg->commdis.bits.unlock_com_err_dis = 0;
    spi_geth_cfg->max_time_out = 638; // 51040ns / 80ns = 638
    spi_geth_cfg->comm_t_set = 6250;
    spi_geth_cfg->monitor_mode = 1;
    spi_geth_cfg->Monitor_Samp_Slow_Mode = 0x00; // 均匀抽点
    spi_geth_cfg->Monitor_Samp_Slow_Point = 0;
    for (int i = 2; i < 3; ++i)
    {
        spi_pcie_cfg->cfg_port_id = i * 2;
        retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(geth_cfg_spi_t));
        if (retVal != 0)
        {
            Debug_logError(" slot1 send geth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send geth[%d] cfg ok!!\n", i);
        }
    }

    /**
     * sfp port 3:接CPE合并单元 10us
     */
    spi_geth_cfg->train_rx_id = 3;
    spi_geth_cfg->commdis.bits.unlock_com_err_dis = 0;
    spi_geth_cfg->max_time_out = 138; // 11040ns / 80ns = 138
    spi_geth_cfg->comm_t_set = 1250; // 1250*8ns = 10000ns
    spi_geth_cfg->monitor_mode = 1; // 收上送
    spi_geth_cfg->Monitor_Samp_Slow_Mode = 0xFF; // 中断前30us抽最新的点
    spi_geth_cfg->Monitor_Samp_Slow_Point = 0;
    for (int i = 3; i < 4; ++i)
    {
        spi_pcie_cfg->cfg_port_id = i * 2;
        retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(geth_cfg_spi_t));
        if (retVal != 0)
        {
            Debug_logError(" slot1 send geth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send geth[%d] cfg ok!!\n", i);
        }
    }

    /**
    * sfp port 4~6：录波板 50us
    * sfp port 7: 千兆备用
    */
    spi_geth_cfg->train_rx_id = 3;
    spi_geth_cfg->commdis.bits.unlock_com_err_dis = 0;
    spi_geth_cfg->max_time_out = 638;// 51040ns / 80ns = 638
    spi_geth_cfg->comm_t_set = 6250;
    spi_geth_cfg->monitor_mode = 1;
    spi_geth_cfg->Monitor_Samp_Slow_Mode = 0x00; // 均匀抽点
    spi_geth_cfg->Monitor_Samp_Slow_Point = 0;
    for (int i = 4; i < 8; ++i)
    {
        spi_pcie_cfg->cfg_port_id = i * 2;
        retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(geth_cfg_spi_t));
        if (retVal != 0)
        {
            Debug_logError(" slot1 send geth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send geth[%d] cfg ok!!\n", i);
        }
    }

    /* ft3 cfg */
    pcie_send_ft3_cfg_spi(0);
    pcie_send_ft3_cfg_spi(1);

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x0;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_INTR;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));

    g_platform_ver.fpga_version.main_soft_ver  = spi_sys_init_bak->soft_ver;
    g_platform_ver.fpga_version.main_fun_ver   = spi_sys_init_bak->fun_ver;
    g_platform_ver.fpga_version.main_pcie_ver  = spi_sys_init_bak->pcie_ver;
    g_platform_ver.fpga_version.main_soft_code = spi_sys_init_bak->soft_check_code;
    g_platform_ver.versionflag |= 0x100;
    Debug_logInfo("soft_ver=0x%04x\n", spi_sys_init_bak->soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", spi_sys_init_bak->fun_ver);
    if (retVal != 0)
    {
        Debug_logError(" slot0 and slot1 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot0 and slot1 send sys init ok!!\n");
    }

    Debug_log("\n");
    Debug_logTag("sending slot0 and slot1 cfg is end\n");
}

void slot2_send_cfg_spi()
{
    int32_t retVal;
    uint16_t cfg_data_offset = 0;
    cfg_data_offset = sizeof(pcie_cfg_head_spi_t) / 2;
    pcie_cfg_head_spi_t *spi_pcie_cfg = (pcie_cfg_head_spi_t *)gCfgBuffer;

    GPIO_write(0, PIN_NUM_SPI_FPGA0, GPIO_PIN_HIGH);
    Debug_log("\n");
    Debug_logTag("sending slot2 pcie cfg...\n");

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x2;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init_t *spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_t *spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_CFG;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));
    if (retVal != 0)
    {
        Debug_logError(" slot2 start send cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot2 start send cfg OK!!\n");
    }

    /* systeam cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_SYS;
    spi_pcie_cfg->cfg_port_id = 0xff;
    spi_pcie_cfg->cfg_slot_id = 0x2;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_config_t) / 2;

    spi_sys_config_t *sys_config = (spi_sys_config_t *)(&gCfgBuffer[cfg_data_offset]);
    train_irq_tim_cfg_spi(sys_config,
                          init_param.core0_main_intr_time,
                          init_param.core1_main_intr_time,
                          init_param.core2_main_intr_time,
                          init_param.core3_main_intr_time,
                          init_param.core4_main_intr_time,
                          init_param.core5_main_intr_time,
                          init_param.core6_main_intr_time,
                          init_param.core7_main_intr_time);
    train_tx_irq_num_cfg_spi(sys_config, 5, 7, 7, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg_spi(sys_config, 5, 7, 7, 1);
    sys_config->slot_id = 2;
    sys_config->ad_samp_ratio = 100000;
    sys_config->soft_err_en = 0;
    sys_config->vcc_fp_en = 0;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_config_t));
    if (retVal != 0)
    {
        Debug_logError(" slot2 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot2 send sys cfg OK!!\n");
    }

    /* eth cfg*/
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_ETH;
    spi_pcie_cfg->cfg_slot_id = 2;
    spi_pcie_cfg->cfg_payload_len = sizeof(eth_cfg_spi_t) / 2;

    eth_cfg_spi_t *spi_eth_cfg = (eth_cfg_spi_t *)(&gCfgBuffer[cfg_data_offset]);

    /* sfp port 0~7 */
    spi_eth_cfg->train_rx_id = 4;
    spi_eth_cfg->forcelink_mode = 0;
    for (int i = 0; i < 8; ++i)
    {
        spi_pcie_cfg->cfg_port_id = i * 2;
        retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(eth_cfg_spi_t));
        if (retVal != 0)
        {
            Debug_logError(" slot2 send eth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot2 send eth[%d] cfg ok!!\n", i);
        }
    }

    /* ft3 cfg */
    pcie_send_ft3_cfg_spi(2);

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x2;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_INTR;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));
    g_platform_ver.fpga_version.slot2_soft_ver  = spi_sys_init_bak->soft_ver;
    g_platform_ver.fpga_version.slot2_fun_ver   = spi_sys_init_bak->fun_ver;
    g_platform_ver.fpga_version.slot2_pcie_ver  = spi_sys_init_bak->pcie_ver;
    g_platform_ver.fpga_version.slot2_soft_code = spi_sys_init_bak->soft_check_code;
    g_platform_ver.versionflag |= 0x200;
    Debug_logInfo("soft_ver=0x%04x\n", spi_sys_init_bak->soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", spi_sys_init_bak->fun_ver);
    if (retVal != 0)
    {
        Debug_logError(" slot2 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot2 send sys init ok!!\n");
    }

    Debug_log("\n");
    Debug_logTag("sending slot2 cfg is end\n");
    GPIO_write(0, PIN_NUM_SPI_FPGA0, GPIO_PIN_LOW);
}

void slot3_send_cfg_spi()
{
    int32_t retVal;
    uint16_t cfg_data_offset = 0;
    cfg_data_offset = sizeof(pcie_cfg_head_spi_t) / 2;
    pcie_cfg_head_spi_t *spi_pcie_cfg = (pcie_cfg_head_spi_t *)gCfgBuffer;

    GPIO_write(0, PIN_NUM_SPI_FPGA1, GPIO_PIN_HIGH);
    Debug_log("\n");
    Debug_logTag("sending slot3 pcie cfg...\n");

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x3;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init_t *spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_t *spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_CFG;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));
    if (retVal != 0)
    {
        Debug_logError(" slot3 start send cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot3 start send cfg OK!!\n");
    }

    /* systeam cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_SYS;
    spi_pcie_cfg->cfg_port_id = 0xff;
    spi_pcie_cfg->cfg_slot_id = 0x3;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_config_t) / 2;

    spi_sys_config_t *sys_config = (spi_sys_config_t *)(&gCfgBuffer[cfg_data_offset]);
    train_irq_tim_cfg_spi(sys_config,
                          init_param.core0_main_intr_time,
                          init_param.core1_main_intr_time,
                          init_param.core2_main_intr_time,
                          init_param.core3_main_intr_time,
                          init_param.core4_main_intr_time,
                          init_param.core5_main_intr_time,
                          init_param.core6_main_intr_time,
                          init_param.core7_main_intr_time);
    train_tx_irq_num_cfg_spi(sys_config, 4, 7, 7, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg_spi(sys_config, 4, 7, 7, 1);
    sys_config->slot_id = 3;
    sys_config->ad_samp_ratio = 100000;
    sys_config->soft_err_en = 0;
    sys_config->vcc_fp_en = 0;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_config_t));
    if (retVal != 0)
    {
        Debug_logError(" slot3 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot3 send sys cfg OK!!\n");
    }

    /* eth cfg*/
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_ETH;
    spi_pcie_cfg->cfg_slot_id = 3;
    spi_pcie_cfg->cfg_payload_len = sizeof(eth_cfg_spi_t) / 2;

    eth_cfg_spi_t *spi_eth_cfg = (eth_cfg_spi_t *)(&gCfgBuffer[cfg_data_offset]);

    /* sfp port 0~7 */
    spi_eth_cfg->train_rx_id = 4;
    spi_eth_cfg->forcelink_mode = 0;
    for (int i = 0; i < 8; ++i)
    {
        spi_pcie_cfg->cfg_port_id = i * 2;
        retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(eth_cfg_spi_t));
        if (retVal != 0)
        {
            Debug_logError(" slot3 send eth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot3 send eth[%d] cfg ok!!\n", i);
        }
    }

    /* ft3 cfg */
    pcie_send_ft3_cfg_spi(3);

    /* init cfg */
    memset(gCfgBuffer, 0, sizeof(gCfgBuffer));
    memset(gCfgBakBuffer, 0, sizeof(gCfgBakBuffer));
    spi_pcie_cfg->head = CPU_TO_FPGA;
    spi_pcie_cfg->cfg_type = CFG_TYPE_INIT;
    spi_pcie_cfg->cfg_port_id = 0xff; //invalid port，do not care
    spi_pcie_cfg->cfg_slot_id = 0x3;
    spi_pcie_cfg->cfg_payload_len = sizeof(spi_sys_init_t) / 2;

    spi_sys_init = (spi_sys_init_t *)(&gCfgBuffer[cfg_data_offset]);
    spi_sys_init_bak = (spi_sys_init_t *)(&gCfgBakBuffer[cfg_data_offset]);
    spi_sys_init->arm_init_ok = INIT_START_INTR;

    retVal = pcie_send_cfg_by_spi(gCfgBuffer, gCfgBakBuffer, sizeof(spi_sys_init_t));
    g_platform_ver.fpga_version.slot3_soft_ver   = spi_sys_init_bak->soft_ver;
    g_platform_ver.fpga_version.slot3_fun_ver    = spi_sys_init_bak->fun_ver;
    g_platform_ver.fpga_version.slot3_pcie_ver   = spi_sys_init_bak->pcie_ver;
    g_platform_ver.fpga_version.slot3_soft_code  = spi_sys_init_bak->soft_check_code;
    g_platform_ver.versionflag |= 0x400;
    Debug_logInfo("soft_ver=0x%04x\n", spi_sys_init_bak->soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", spi_sys_init_bak->fun_ver);
    if (retVal != 0)
    {
        Debug_logError(" slot3 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot3 send sys init ok!!\n");
    }

    Debug_log("\n");
    Debug_logTag("sending slot3 cfg is end\n");
    GPIO_write(0, PIN_NUM_SPI_FPGA1, GPIO_PIN_LOW);
}
