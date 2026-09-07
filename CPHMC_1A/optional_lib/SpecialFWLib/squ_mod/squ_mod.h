/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       squ_mod.h
 *@author     wenjunf
 *@date       2026.01.14
 *@brief      方波调制算法块接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.14  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _SQU_MOD_H
#define _SQU_MOD_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "pcie_fpga.h"
#include "pcie_spi_cfg.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct _squ_mod_bit_def
{
    /**
     * Bit0~2：槽位号0~6
     * Bit3~7：端口号0~31
     * 当槽位号为7时该通道发送功能屏蔽
     */
    uint8_t  slot:3;
    uint8_t  port:5;
} squ_mod_bit_def_t;

typedef struct _squ_mod_cfg
{
    squ_mod_bit_def_t  tx_squ_mod_port[4];
    squ_mod_bit_def_t  rx_squ_mod_port[4];
    uint8_t  rx_squ_filt_time;
} __attribute__((packed)) squ_mod_cfg_t;

typedef struct _squ_mod_cfg_chk
{
    uint32_t  cfg_state;
    squ_mod_cfg_t cfg_data;
    squ_mod_cfg_t cfg_data_bak;
} __attribute__((packed)) squ_mod_cfg_chk_t;

typedef struct _squ_diag_info
{
    uint32_t squ_wav_errs;
    uint32_t squ_wav_sta_errs;
    uint32_t header_car_errs;
} squ_diag_info_t;

extern squ_mod_cfg_chk_t g_squ_mod_cfg_all;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void set_squ_mod_cfg(squ_mod_cfg_t *cfg);
uint32_t get_squ_mod_cfg(squ_mod_cfg_t *cfg);
void send_squ_mod_cmd(uint8_t *p_squmod_data);
squ_diag_info_t *recv_squ_wav(uint8_t *rx_squ_wav, uint8_t *rx_squ_wav_sta);

#ifndef USE_SPI_CFG

void pcie_set_squ_mod_cfg_2_syscfg(sys_config_t *p_sys_cfg);
void pcie_get_syscfg_2_squ_mod_cfg(sys_config_t *p_sys_cfg, uint32_t cfg_state);
#else
void pcie_set_squ_mod_cfg_2_syscfg(spi_sys_config_t *p_sys_cfg);
void pcie_get_syscfg_2_squ_mod_cfg(spi_sys_config_t *p_sys_cfg, uint32_t cfg_state);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _SQU_MOD_H */
