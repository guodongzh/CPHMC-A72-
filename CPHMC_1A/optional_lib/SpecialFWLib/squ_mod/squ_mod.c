/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       squ_mod.c
 *@author     wenjunf
 *@date       2026.01.14
 *@brief      方波调制算法块接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.14  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "squ_mod.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
squ_mod_cfg_chk_t g_squ_mod_cfg_all __attribute__((section(".squ_mod_cfg")));
squ_diag_info_t g_squ_mod_diag_info = {0};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
void set_squ_mod_cfg(squ_mod_cfg_t *cfg)
{
    // FIXME 需要根据实际端口配置：判断配置的端口是否合法
    memcpy(&g_squ_mod_cfg_all.cfg_data, cfg, sizeof(squ_mod_cfg_t));
    g_squ_mod_cfg_all.cfg_state = 1;
}

uint32_t get_squ_mod_cfg(squ_mod_cfg_t *cfg)
{
    CacheP_Inv(&g_squ_mod_cfg_all, sizeof(g_squ_mod_cfg_all));
    memcpy(cfg, &g_squ_mod_cfg_all.cfg_data_bak, sizeof(squ_mod_cfg_t));
    return g_squ_mod_cfg_all.cfg_state;
}
void send_squ_mod_cmd(uint8_t *p_squmod_data)
{
    uint8_t *p_cmd_data = &slot0_1_train_tx[0].header->cmd[0]; // 特快索引车
    p_cmd_data[0] = *p_squmod_data;
    // 计算反码: ~chx_squ_mod_cmd
    p_cmd_data[1] = ~(p_cmd_data[0]);
}

squ_diag_info_t* recv_squ_wav(uint8_t *rx_squ_wav, uint8_t *rx_squ_wav_sta)
{
    uint8_t *p_sys_sta_data = &slot0_1_train_rx[0].header->sys_status_data[0];
    *rx_squ_wav = p_sys_sta_data[0];
    *rx_squ_wav_sta = p_sys_sta_data[2];
    uint8_t chx_squ_wav_chk = p_sys_sta_data[1];
    uint8_t chx_squ_wav_sta_chk = p_sys_sta_data[3];
    uint8_t chx_calc_wav_chk = ~p_sys_sta_data[0];
    uint8_t chx_calc_wav_sta_chk = ~p_sys_sta_data[2];

    // 校验反码
    if (chx_squ_wav_chk != chx_calc_wav_chk)
    {
        g_squ_mod_diag_info.squ_wav_errs++;
    }
    if (chx_squ_wav_sta_chk != chx_calc_wav_sta_chk)
    {
        g_squ_mod_diag_info.squ_wav_sta_errs++;
    }
    g_squ_mod_diag_info.header_car_errs = slot0_1_train_rx[0].header_sum_err;

    return &g_squ_mod_diag_info;
}

#ifndef USE_SPI_CFG
void pcie_set_squ_mod_cfg_2_syscfg(sys_config_t *p_sys_cfg)
{
    memcpy(&p_sys_cfg->tx_squ_mod_port[0], &g_squ_mod_cfg_all.cfg_data, sizeof(squ_mod_cfg_t));
}

void pcie_get_syscfg_2_squ_mod_cfg(sys_config_t *p_sys_cfg, uint32_t cfg_state)
{
    memcpy(&g_squ_mod_cfg_all.cfg_data_bak, &p_sys_cfg->tx_squ_mod_port[0], sizeof(squ_mod_cfg_t));
    g_squ_mod_cfg_all.cfg_state = cfg_state;
}

#else
void pcie_set_squ_mod_cfg_2_syscfg(spi_sys_config_t *p_sys_cfg)
{
    memcpy(&p_sys_cfg->tx_squ_mod_port[0], &g_squ_mod_cfg_all.cfg_data, sizeof(squ_mod_cfg_t));
}

void pcie_get_syscfg_2_squ_mod_cfg(spi_sys_config_t *p_sys_cfg, uint32_t cfg_state)
{
    memcpy(&g_squ_mod_cfg_all.cfg_data_bak, &p_sys_cfg->tx_squ_mod_port[0], sizeof(squ_mod_cfg_t));
    g_squ_mod_cfg_all.cfg_state = cfg_state;
    CacheP_wb(&g_squ_mod_cfg_all, sizeof(g_squ_mod_cfg_all));
}
#endif