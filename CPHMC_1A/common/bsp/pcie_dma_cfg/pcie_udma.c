/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_udma.c
 *@author     LiuRui
 *@date       2025.09.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.25  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pcie_fpga.h"

#ifndef USE_SPI_CFG
#include <udma_mem_copy.h>
#include "squ_mod.h"
#include <debug_config.h>
#include <ft3_data.h>
#include <udma_mem_copy.h>
#include <squ_mod.h>
#include "pcie_udma.h"
#include "bsp_init.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
eth_cfg_t geth_cfg_bak __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT))) = {0};
eth_cfg_t geth_cfg __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT))) = {0};

aurora_cfg_t aurora_data_cfg_bak __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT))) = {0};
aurora_cfg_t aurora_data_cfg __attribute__((aligned(UDMA_CACHELINE_ALIGNMENT))) = {0};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
int memcmp_(const void *cs, const void *ct, size_t n)
{
    if (n)
    {
        const unsigned char *mem1 = (unsigned char *)cs;
        const unsigned char *mem2 = (unsigned char *)ct;
        int cp1, cp2;

        while ((cp1 = *mem1++) == (cp2 = *mem2++) && --n)
            ;
        return cp1 - cp2;
    }
    return 0;
}

void memcpy_(void *__restrict to, const void *__restrict from, size_t n)
{
    uint32_t *dest = to, *src = from;
    for (int i = 0; i < n / 4; ++i)
    {
        *dest = *src;
        src++;
        dest++;
    }
}

int pcie_send_cfg_with_dma(const void *cfg, void *cfg_bak, uint64_t dest_addr, uint32_t len)
{
    int32_t retVal;

    /*send cfg*/
    CacheP_wb(cfg, len);
    retVal = udma_memcpy((void*)dest_addr, cfg, len);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" pcie udma send failed !!\n");
        return retVal;
    }

    retVal = udma_memcpy_wait_complete(0xffffffff);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" pcie udma send failed !!\n");
        return retVal;
    }

    /*get cfg*/
    retVal = udma_memcpy(cfg_bak, (void *)dest_addr, len);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" pcie udma send failed !!\n");
        return retVal;
    }
    retVal = udma_memcpy_wait_complete(0xffffffff);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" pcie udma send failed !!\n");
        return retVal;
    }

    /*check cfg*/
    CacheP_Inv(cfg_bak, len);
    uint16_t fream_head = *(uint16_t *)cfg_bak;
    if (fream_head == FPGA_TO_CPU)
    {
        uint32_t *data;
        uint32_t sum_check = 0;
        uint32_t *sum_check_bak = (uint32_t *)((uint8_t *)cfg_bak + len - 4);
        data = (uint32_t *)cfg_bak;
        for (int i = 0; i < len / 4 - 1; ++i)
        {
            sum_check += data[i];
        }
        if (sum_check == *sum_check_bak)
        {
            if (memcmp_((uint8_t *)cfg + 2, (uint8_t *)cfg_bak + 2, len - 6) != 0)
            {
                Debug_logError(" cfg is not match!! ");
                Debug_log("please check cfg!!\n");
                return UDMA_EFAIL;
            }
            else
            {
                return UDMA_SOK;
            }
        }
    }
    return UDMA_EFAIL;
}

int pcie_send_cfg_by_cpu(const void *cfg, void *cfg_bak, uint64_t dest_addr, uint32_t len)
{
    void *dest = (void *)dest_addr;
    /*send cfg*/
    memcpy_(dest, cfg, len);
    CacheP_wb(dest, len);

    Osal_delay(100);

    /*get cfg*/
    CacheP_Inv(dest, len);
    memcpy_(cfg_bak, dest, len);

    /*check cfg*/
    uint16_t fream_head = *(uint16_t *)cfg_bak;
    if (fream_head == FPGA_TO_CPU)
    {
        uint32_t *data;
        uint32_t sum_check = 0;
        uint32_t *sum_check_bak = (uint32_t *)((uint8_t *)cfg_bak + len - 4);
        data = (uint32_t *)cfg_bak;
        for (int i = 0; i < len / 4 - 1; ++i)
        {
            sum_check += data[i];
        }
        if (sum_check == *sum_check_bak)
        {
            if (memcmp_((uint8_t *)cfg + 2, (uint8_t *)cfg_bak + 2, len - 6) != 0)
            {
                Debug_logError(" cfg is not match!! ");
                Debug_log("please try again!!\n");
                return UDMA_EFAIL;
            }
            else
            {
                return UDMA_SOK;
            }
        }
    }
    return UDMA_EFAIL;
}

/**
 * @param slot_num  slot number(0~3)
 */
void pcie_send_ft3_cfg(uint32_t slot_num)
{
    int32_t retVal;
    uint64_t dest_addr;
    uint64_t base_addr[FT3_SOLT_NUM][2] = {
        {
            PCIE0_FT3_TX1_CFG_ADDR,
            PCIE0_FT3_RX1_CFG_ADDR,
        },
        {
            PCIE0_FT3_TX1_CFG_ADDR,
            PCIE0_FT3_RX1_CFG_ADDR,
        },
        {
            PCIE3_FT3_TX1_CFG_ADDR,
            PCIE3_FT3_RX1_CFG_ADDR,
        },
        {
            PCIE2_FT3_TX1_CFG_ADDR,
            PCIE2_FT3_RX1_CFG_ADDR,
        },
    };

    for (int ch_num = 0; ch_num < FT3_TX_CH_NUM; ++ch_num)
    {
        /*Is the parameter valid?*/
        if (ft3_tx_cfg_all[slot_num][ch_num].cfg_state == CFG_VALID)
        {
            if (slot_num == 0 || slot_num == 1)
            {
                if (ch_num >= 16)
                {
                    dest_addr = base_addr[slot_num][0] + (FT3_CH_CFG_OFFSET * (ch_num - 8));
                }
                else
                {
                    dest_addr = base_addr[slot_num][0] + (FT3_CH_CFG_OFFSET * ch_num);
                }
                retVal = pcie_send_cfg_with_dma(&(ft3_tx_cfg_all[slot_num][ch_num].cfg_data),
                                                &(ft3_tx_cfg_all[slot_num][ch_num].cfg_data_bak),
                                                dest_addr,
                                                sizeof(ft3_cfg_t));
            }
            else
            {
                if (ch_num >= 16)
                {
                    dest_addr = base_addr[slot_num][0] + (FT3_CH_CFG_OFFSET * (ch_num - 16));
                }
                else
                {
                    dest_addr = base_addr[slot_num][0] + (FT3_CH_CFG_OFFSET * ch_num);
                }
                retVal = pcie_send_cfg_by_cpu(&(ft3_tx_cfg_all[slot_num][ch_num].cfg_data),
                                              &(ft3_tx_cfg_all[slot_num][ch_num].cfg_data_bak),
                                              dest_addr,
                                              sizeof(ft3_cfg_t));
            }

            if (retVal == 0)
            {
                ft3_tx_cfg_all[slot_num][ch_num].cfg_state = CFG_PASS;
                Debug_logOk(" SLOT[%d]-CH:[%d] ft3-tx cfg OK!!\n", slot_num, ch_num);
            }
            else
            {
                ft3_tx_cfg_all[slot_num][ch_num].cfg_state = CFG_FAIL;
                Debug_logError(" SLOT[%d]-CH:[%d] ft3-tx cfg failed!!\n", slot_num, ch_num);
            }
        }
    }

    for (int ch_num = 0; ch_num < FT3_RX_CH_NUM; ++ch_num)
    {
        /*Is the parameter valid?*/
        if (ft3_rx_cfg_all[slot_num][ch_num].cfg_state == CFG_VALID)
        {
            if (slot_num == 0 || slot_num == 1)
            {
                if (ch_num >= 16)
                {
                    dest_addr = base_addr[slot_num][1] + (FT3_CH_CFG_OFFSET * (ch_num - 8));
                }
                else
                {
                    dest_addr = base_addr[slot_num][1] + (FT3_CH_CFG_OFFSET * ch_num);
                }
                retVal = pcie_send_cfg_with_dma(&ft3_rx_cfg_all[slot_num][ch_num].cfg_data,
                                                &ft3_rx_cfg_all[slot_num][ch_num].cfg_data_bak,
                                                dest_addr,
                                                sizeof(ft3_cfg_t));
            }
            else
            {
                if (ch_num >= 16)
                {
                    dest_addr = base_addr[slot_num][1] + (FT3_CH_CFG_OFFSET * (ch_num - 16));
                }
                else
                {
                    dest_addr = base_addr[slot_num][1] + (FT3_CH_CFG_OFFSET * ch_num);
                }
                retVal = pcie_send_cfg_by_cpu(&ft3_rx_cfg_all[slot_num][ch_num].cfg_data,
                                              &ft3_rx_cfg_all[slot_num][ch_num].cfg_data_bak,
                                              dest_addr,
                                              sizeof(ft3_cfg_t));
            }

            if (retVal == 0)
            {
                ft3_rx_cfg_all[slot_num][ch_num].cfg_state = CFG_PASS;
                Debug_logOk(" SLOT[%d]-CH:[%d] ft3-rx cfg OK!!\n", slot_num, ch_num);
            }
            else
            {
                ft3_rx_cfg_all[slot_num][ch_num].cfg_state = CFG_FAIL;
                Debug_logError(" SLOT[%d]-CH:[%d] ft3-rx cfg failed!!\n", slot_num, ch_num);
            }
        }
    }
}

int slot0_1_fpga_init(sys_config_t *sys_config)
{
    uint32_t end_addr;
    uint32_t *data = (uint32_t *)sys_config;
    uint32_t buff_end_addr = (uint32_t)pcie0_ib_space + sizeof(pcie0_ib_space);

    train_irq_tim_cfg(sys_config,
                      init_param.core0_main_intr_time,
                      init_param.core1_main_intr_time,
                      init_param.core2_main_intr_time,
                      init_param.core3_main_intr_time,
                      init_param.core4_main_intr_time,
                      init_param.core5_main_intr_time,
                      init_param.core6_main_intr_time,
                      init_param.core7_main_intr_time);
    train_tx_irq_num_cfg(sys_config, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg(sys_config, 7, 7, 7, 1);
    train_xdma_time_cfg(sys_config, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sys_config->slot_id = 0;
    sys_config->ad_samp_ratio = 5000;
    pcie_set_squ_mod_cfg_2_syscfg(sys_config);

#if USE_C66x_RAM
    end_addr = train_tx_pcie_addr_init(&sys_config->tx_car[0],
                                       &slot0_1_train_tx[0],
                                       1,
                                       PCIE0_IB1_PCIE_ADDR);
    train_rx_pcie_addr_init(&sys_config->rx_car[0], &slot0_1_train_rx[0], 1, end_addr);

    end_addr = train_tx_pcie_addr_init(&sys_config->tx_car[1],
                                       &slot0_1_train_tx[1],
                                       (sizeof(slot0_1_train_tx) / sizeof(train_tx_t)) - 1,
                                       (uint32_t)pcie0_ib_space);
    train_rx_pcie_addr_init(&sys_config->rx_car[1],
                            &slot0_1_train_rx[1],
                            (sizeof(slot0_1_train_rx) / sizeof(train_rx_t)) - 1,
                            end_addr);
#else

    end_addr = train_tx_pcie_addr_init(&sys_config->tx_car[0],
                                       &slot0_1_train_tx[0],
                                       (sizeof(slot0_1_train_tx) / sizeof(train_tx_t)),
                                       (uint32_t)pcie0_ib_space);
    train_rx_pcie_addr_init(&sys_config->rx_car[0],
                            &slot0_1_train_rx[0],
                            (sizeof(slot0_1_train_rx) / sizeof(train_rx_t)),
                            end_addr);
#endif

    if (end_addr > buff_end_addr)
    {
        Debug_logError("Failed to allocate PCIe0 data buffer!!\n");
    }
    else
    {
        Debug_logInfo("Allocate PCIe0 data buffer successfully!!\n");
    }

    sys_config->fream_head = CPU_TO_FPGA;
    for (int i = 0; i < (sizeof(sys_config_t) / 4) - 1; ++i)
    {
        sys_config->sum_check += data[i];
    }

    return 0;
}

int slot2_fpga_init(sys_config_t *sys_config)
{
    uint32_t end_addr;
    uint32_t *data = (uint32_t *)sys_config;
    uint32_t buff_end_addr = (uint32_t)pcie3_ib_space + sizeof(pcie3_ib_space);

    train_irq_tim_cfg(sys_config,
                      init_param.core0_main_intr_time,
                      init_param.core1_main_intr_time,
                      init_param.core2_main_intr_time,
                      init_param.core3_main_intr_time,
                      init_param.core4_main_intr_time,
                      init_param.core5_main_intr_time,
                      init_param.core6_main_intr_time,
                      init_param.core7_main_intr_time);
    train_tx_irq_num_cfg(sys_config, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg(sys_config, 7, 7, 7, 1);
    train_xdma_time_cfg(sys_config, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sys_config->slot_id = 2;
    sys_config->ad_samp_ratio = 5000;

    end_addr = train_tx_pcie_addr_init(&sys_config->tx_car[0],
                                       &slot2_train_tx[0],
                                       (sizeof(slot2_train_tx) / sizeof(train_tx_t)),
                                       (uint32_t)pcie3_ib_space);
    train_rx_pcie_addr_init(&sys_config->rx_car[0],
                            &slot2_train_rx[0],
                            (sizeof(slot2_train_rx) / sizeof(train_rx_t)),
                            end_addr);

    if (end_addr > buff_end_addr)
    {
        Debug_logError("Failed to allocate PCIe3 data buffer!!\n");
    }
    else
    {
        Debug_logInfo("Allocate PCIe3 data buffer successfully!!\n");
    }

    sys_config->fream_head = CPU_TO_FPGA;
    for (int i = 0; i < (sizeof(sys_config_t) / 4) - 1; ++i)
    {
        sys_config->sum_check += data[i];
    }

    return 0;
}

int slot3_fpga_init(sys_config_t *sys_config)
{
    uint32_t end_addr;
    uint32_t *data = (uint32_t *)sys_config;
    uint32_t buff_end_addr = (uint32_t)pcie2_ib_space + sizeof(pcie2_ib_space);

    train_irq_tim_cfg(sys_config,
                      init_param.core0_main_intr_time,
                      init_param.core1_main_intr_time,
                      init_param.core2_main_intr_time,
                      init_param.core3_main_intr_time,
                      init_param.core4_main_intr_time,
                      init_param.core5_main_intr_time,
                      init_param.core6_main_intr_time,
                      init_param.core7_main_intr_time);
    train_tx_irq_num_cfg(sys_config, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1);
    train_rx_irq_num_cfg(sys_config, 7, 7, 7, 1);
    train_xdma_time_cfg(sys_config, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sys_config->slot_id = 3;
    sys_config->ad_samp_ratio = 5000;

    end_addr = train_tx_pcie_addr_init(&sys_config->tx_car[0],
                                       &slot3_train_tx[0],
                                       (sizeof(slot3_train_tx) / sizeof(train_tx_t)),
                                       (uint32_t)pcie2_ib_space);
    train_rx_pcie_addr_init(&sys_config->rx_car[0],
                            &slot3_train_rx[0],
                            (sizeof(slot3_train_rx) / sizeof(train_rx_t)),
                            end_addr);

    if (end_addr > buff_end_addr)
    {
        Debug_logError("Failed to allocate PCIe2 data buffer!!\n");
    }
    else
    {
        Debug_logInfo("Allocate PCIe2 data buffer successfully!!\n");
    }

    sys_config->fream_head = CPU_TO_FPGA;
    for (int i = 0; i < (sizeof(sys_config_t) / 4) - 1; ++i)
    {
        sys_config->sum_check += data[i];
    }

    return 0;
}

void slot0_1_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak)
{
    int32_t retVal;
    Debug_log("\nsending slot0 and slot1 pcie cfg...\n");

    /*for calc check sum*/
    uint32_t *data;
    uint32_t check_sum;

    /*sys_cfg*/
    retVal = pcie_send_cfg_with_dma(sys_config,
                                    sys_config_bak,
                                    PCIE0_SYS_CFG_ADDR,
                                    sizeof(sys_config_t));
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot0 and slot1 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot0 and slot1 send sys cfg OK!!\n");
#ifndef USE_SPI_CFG
        pcie_get_syscfg_2_squ_mod_cfg(sys_config_bak, UDMA_SOK != retVal);
#endif
    }

    /*geth_init*/
    geth_cfg.fream_head = CPU_TO_FPGA;
    geth_cfg.train_rx_id = 3;

    data = (uint32_t *)&geth_cfg;
    check_sum = 0;
    for (int i = 0; i < sizeof(eth_cfg_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    geth_cfg.check_sum = check_sum;

    /* sfp port 0,1 */
    for (int i = 0; i < 2; ++i)
    {
        retVal = pcie_send_cfg_with_dma(&geth_cfg,
                                        &geth_cfg_bak,
                                        PCIE0_GETH1_CFG_ADDR + (GETH_CFG_OFFSET * i),
                                        sizeof(eth_cfg_t));
        if (UDMA_SOK != retVal)
        {
            Debug_logError(" slot1 send geth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send geth[%d] cfg ok!!\n", i);
        }
    }

    /*geth_init*/
    geth_cfg.fream_head = CPU_TO_FPGA;
    geth_cfg.train_rx_id = 3;

    data = (uint32_t *)&geth_cfg;
    check_sum = 0;
    for (int i = 0; i < sizeof(eth_cfg_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    geth_cfg.check_sum = check_sum;

    /* sfp port 4,5 */
    for (int i = 4; i < 6; ++i)
    {
        retVal = pcie_send_cfg_with_dma(&geth_cfg,
                                        &geth_cfg_bak,
                                        PCIE0_GETH1_CFG_ADDR + (GETH_CFG_OFFSET * i),
                                        sizeof(eth_cfg_t));
        if (UDMA_SOK != retVal)
        {
            Debug_logError(" slot1 send geth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send geth[%d] cfg ok!!\n", i);
        }
    }

    /*aurora_init*/
    aurora_data_cfg.fream_head = CPU_TO_FPGA;
    aurora_data_cfg.train_rx_id = 3;

    data = (uint32_t *)&aurora_data_cfg;
    check_sum = 0;
    for (int i = 0; i < sizeof(aurora_cfg_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    aurora_data_cfg.check_sum = check_sum;

    /* sfp port 6,7 */
    for (int i = 0; i < 2; ++i)
    {
        retVal = pcie_send_cfg_with_dma(&aurora_data_cfg,
                                        &aurora_data_cfg_bak,
                                        AURORA_CFG_ADDR + (AURORA_CFG_OFFSET * i),
                                        sizeof(aurora_cfg_t));
        if (UDMA_SOK != retVal)
        {
            Debug_logError(" slot1 send aurora[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot1 send aurora[%d] cfg ok!!\n", i);
        }
    }

#if 0
       /*ft3_init*/
       ft3_cfg_t ft3_cfg = {0};
       ft3_cfg_t ft3_cfg_bak = {0};
       ft3_cfg.frame_head = CPU_TO_FPGA;
       ft3_cfg.train_rx_id = 1;
       ft3_cfg.blk_num = 2;
       ft3_cfg.last_blk_len = 18;
       ft3_cfg.frm_data_len = 38;
       ft3_cfg.max_time_out = 62500;
       ft3_cfg.comm_t_set = 62500;
       ft3_cfg.bps_div = 20;
       ft3_cfg.unlock_com_err_dis = 1;
       ft3_cfg.polset = 0;
       ft3_cfg.samp_slow = 0;
       data = (uint32_t *)&ft3_cfg;
       check_sum = 0;
       for (int i = 0; i < sizeof(sys_init_t) / 4 - 1; ++i)
       {
           check_sum += data[i];
       }
       ft3_cfg.check_sum = check_sum;

       for (int i = 0; i < 24; ++i)
       {
           uint64_t base_addr = PCIE0_FT3_TX1_CFG_ADDR;
           if (i % 2 != 0)
               base_addr = PCIE0_FT3_RX1_CFG_ADDR;
           retVal = pcie_send_cfg_with_dma(chHandle, &ft3_cfg,
                                           &ft3_cfg_bak,
                                           (base_addr + (FT3_CH_CFG_OFFSET * i)),
                                           sizeof(ft3_cfg_t));
           if (UDMA_SOK != retVal)
           {
               Debug_logError(" pcie0 send ft3[%d] cfg failed!!\n", i);
           }
           else
           {
               Debug_logOk(" pcie0 send ft3[%d] cfg ok!!\n", i);
           }
       }
#endif

    pcie_send_ft3_cfg(0);
    pcie_send_ft3_cfg(1);

    /*sys_init*/
    sys_init_t sys_init = {0};
    sys_init_t sys_init_bak = {0};
    sys_init.fream_head = CPU_TO_FPGA;
    sys_init.arm_init_ok = 0xaa55;
    sys_init.soft_ver = 0x3009;
    sys_init.fun_ver = 0xf001;
    sys_init.soft_code = 0x11112222;

    data = (uint32_t *)&sys_init;
    check_sum = 0;
    for (int i = 0; i < sizeof(sys_init_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    sys_init.check_sum = check_sum;

    retVal = pcie_send_cfg_with_dma(&sys_init,
                                    &sys_init_bak,
                                    PCIE0_SYS_INIT_ADDR,
                                    sizeof(sys_init_t));
    g_platform_ver.fpga_version.main_soft_ver = sys_init_bak.soft_ver;
    g_platform_ver.fpga_version.main_fun_ver = sys_init_bak.fun_ver;
    Debug_logInfo("soft_ver=0x%04x\n", sys_init_bak.soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", sys_init_bak.fun_ver);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot0 and slot1 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot0 and slot1 send sys init ok!!\n");
    }

    Debug_log("\nsending slot0 and slot1 cfg is end\n");
}

void slot2_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak)
{
    int32_t retVal;

    Debug_log("\nsending slot2 cfg...\n");

    /*for calc check sum*/
    uint32_t *data;
    uint32_t check_sum;

    /*sys_cfg*/
    retVal =
        pcie_send_cfg_by_cpu(sys_config, sys_config_bak, PCIE3_SYS_CFG_ADDR, sizeof(sys_config_t));
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot2 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot2 send sys cfg OK!!\n");
    }

    /*eth_init*/
    geth_cfg.fream_head = CPU_TO_FPGA;
    geth_cfg.train_rx_id = 4;

    data = (uint32_t *)&geth_cfg;
    check_sum = 0;
    for (int i = 0; i < sizeof(eth_cfg_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    geth_cfg.check_sum = check_sum;

    for (int i = 0; i < 8; ++i)
    {
        retVal = pcie_send_cfg_by_cpu(&geth_cfg,
                                      &geth_cfg_bak,
                                      PCIE3_ETH1_CFG_ADDR + (ETH_CFG_OFFSET * i),
                                      sizeof(eth_cfg_t));
        if (UDMA_SOK != retVal)
        {
            Debug_logError(" slot2 send eth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot2 send eth[%d] cfg ok!!\n", i);
        }
    }

    pcie_send_ft3_cfg(2);

    /*sys_init*/
    sys_init_t sys_init = {0};
    sys_init_t sys_init_bak = {0};
    sys_init.fream_head = CPU_TO_FPGA;
    sys_init.arm_init_ok = 0xaa55;
    sys_init.soft_ver = 0x2506;
    sys_init.fun_ver = 0xce02;
    sys_init.soft_code = 0x11112222;

    data = (uint32_t *)&sys_init;
    check_sum = 0;
    for (int i = 0; i < sizeof(sys_init_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    sys_init.check_sum = check_sum;

    retVal = pcie_send_cfg_by_cpu(&sys_init, &sys_init_bak, PCIE3_SYS_INIT_ADDR, sizeof(sys_init_t));
    g_platform_ver.fpga_version.slot2_soft_ver = sys_init_bak.soft_ver;
    g_platform_ver.fpga_version.slot2_fun_ver = sys_init_bak.fun_ver;
    Debug_logInfo("soft_ver=0x%04x\n", sys_init_bak.soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", sys_init_bak.fun_ver);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot2 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot2 send sys init ok!!\n");
    }

    Debug_log("\nsending slot2 cfg is end\n");
}

void slot3_send_all_cfg(sys_config_t *sys_config, sys_config_t *sys_config_bak)
{
    int32_t retVal;

    Debug_log("\nsending slot3 cfg...\n");

    /*for calc check sum*/
    uint32_t *data;
    uint32_t check_sum;

    /*sys_cfg*/
    retVal = pcie_send_cfg_by_cpu(sys_config, sys_config_bak,
                                  PCIE2_SYS_CFG_ADDR,
                                  sizeof(sys_config_t));
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot3 send sys cfg failed!!\n");
    }
    else
    {
        Debug_logOk(" slot3 send sys cfg OK!!\n");
    }

    /*eth_init*/
    geth_cfg.fream_head = CPU_TO_FPGA;
    geth_cfg.train_rx_id = 4;

    data = (uint32_t *)&geth_cfg;
    check_sum = 0;
    for (int i = 0; i < sizeof(eth_cfg_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    geth_cfg.check_sum = check_sum;

    for (int i = 0; i < 8; ++i)
    {
        retVal = pcie_send_cfg_by_cpu(&geth_cfg,
                                      &geth_cfg_bak,
                                      PCIE2_ETH1_CFG_ADDR + (ETH_CFG_OFFSET * i),
                                      sizeof(eth_cfg_t));
        if (UDMA_SOK != retVal)
        {
            Debug_logError(" slot3 send eth[%d] cfg failed!!\n", i);
        }
        else
        {
            Debug_logOk(" slot3 send eth[%d] cfg ok!!\n", i);
        }
    }

    pcie_send_ft3_cfg(3);

    /*sys_init*/
    sys_init_t sys_init = {0};
    sys_init_t sys_init_bak = {0};
    sys_init.fream_head = CPU_TO_FPGA;
    sys_init.arm_init_ok = 0xaa55;
    sys_init.soft_ver = 0x2506;
    sys_init.fun_ver = 0xce02;
    sys_init.soft_code = 0x11112222;

    data = (uint32_t *)&sys_init;
    check_sum = 0;
    for (int i = 0; i < sizeof(sys_init_t) / 4 - 1; ++i)
    {
        check_sum += data[i];
    }
    sys_init.check_sum = check_sum;

    retVal = pcie_send_cfg_by_cpu(&sys_init, &sys_init_bak, PCIE2_SYS_INIT_ADDR, sizeof(sys_init_t));
    g_platform_ver.fpga_version.slot3_soft_ver = sys_init_bak.soft_ver;
    g_platform_ver.fpga_version.slot3_fun_ver = sys_init_bak.fun_ver;
    Debug_logInfo("soft_ver=0x%04x\n", sys_init_bak.soft_ver);
    Debug_logInfo("fun_ver=0x%04x\n", sys_init_bak.fun_ver);
    if (UDMA_SOK != retVal)
    {
        Debug_logError(" slot3 send sys init failed!!\n");
    }
    else
    {
        Debug_logOk(" slot3 send sys init ok!!\n");
    }

    Debug_log("\nsending slot3 cfg is end\n");
}

#endif
