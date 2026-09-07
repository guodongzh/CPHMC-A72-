/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       fpga_monitor_info.h
 *@author     LiuRui
 *@date       2026.04.09
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.04.09  1.0       LiuRui
 ******************************************************************************/

#ifndef __FPGA_MONITOR_INFO_H
#define __FPGA_MONITOR_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pcie_fpga.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define MAX_FPGA_GEN_MONITOR_NUMBER 64
#define MAX_FPGA_COM_MONITOR_NUMBER 32
#define MAX_FPGA_GEN_NUM 4

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct _local_monitor_info
{
    uint32_t fpga_err_cnt_delta;
    uint32_t old_fpga_err_cnt;
} local_monitor_info_t;

typedef struct _local_fpga_monitor_info
{
    local_monitor_info_t local_gen_info[MAX_FPGA_GEN_MONITOR_NUMBER * 2];
    local_monitor_info_t local_Meth_info[MAX_FPGA_COM_MONITOR_NUMBER];
    local_monitor_info_t local_Geth_info[MAX_FPGA_COM_MONITOR_NUMBER];
    local_monitor_info_t local_aurora_info[MAX_FPGA_COM_MONITOR_NUMBER];
    local_monitor_info_t local_sw_info[MAX_FPGA_COM_MONITOR_NUMBER];
    local_monitor_info_t local_ft3_info[MAX_FPGA_COM_MONITOR_NUMBER];
} local_fpga_monitor_info_t;

// FPGA监视信息
typedef struct _fpga_gen_com_monitor_info
{
    uint16_t slot_id;           // 插件序号
    uint16_t fpga_err_code;     // 错误码
    uint32_t fpga_err_cnt;      // FPGA通用故障计数,若发生故障则计数器+1
    uint16_t src_module_pwr;    // 原始光模块光功率值, FPGAh核心温度
    float module_pwr_status;    // 光模块光功率码值,
    uint32_t old_fpga_err_cnt;  // FPGA通用故障计数,上一次的值
} fpga_gen_com_monitor_inf_t;

typedef struct _fpga_monitor_inf
{
    fpga_gen_com_monitor_inf_t fpga_gen_monitor[MAX_FPGA_GEN_NUM][MAX_FPGA_GEN_MONITOR_NUMBER * 2];  // 通用监视信息(与端口无关)
    fpga_gen_com_monitor_inf_t fpga_Meth_monitor[ETH_SOLT_NUM][MAX_FPGA_COM_MONITOR_NUMBER];  // 百兆网口监视信息
    fpga_gen_com_monitor_inf_t fpga_Geth_monitor[GETH_SOLT_NUM][MAX_FPGA_COM_MONITOR_NUMBER];  // 千兆网口监视信息
    fpga_gen_com_monitor_inf_t fpga_aurora_monitor[AURORA_SLOT_NUM][MAX_FPGA_COM_MONITOR_NUMBER];  // Aurora端口监视信息
    fpga_gen_com_monitor_inf_t fpga_sw_monitor[MAX_FPGA_COM_MONITOR_NUMBER];  // 扩展脉冲箱端口监视信息
    fpga_gen_com_monitor_inf_t fpga_ft3_monitor[FT3_SOLT_NUM][MAX_FPGA_COM_MONITOR_NUMBER];  // FT3端口监视信息
} fpga_monitor_inf_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern fpga_monitor_inf_t all_fpga_monitor_inf;

void fpga_monitor_info_init(void);
void fpga_monitor_info_update(train_header_rx_t *header);

#ifdef __cplusplus
}
#endif

#endif  //__FPGA_MONITOR_INFO_H