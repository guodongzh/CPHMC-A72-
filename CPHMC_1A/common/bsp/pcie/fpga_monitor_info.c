/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       fpga_monitor_info.c
 *@author     LiuRui
 *@date       2026.04.09
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.04.09  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <math.h>
#include "fpga_monitor_info.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
fpga_monitor_inf_t all_fpga_monitor_inf __attribute__((section(".fpga_monitor_shm_section")));
local_fpga_monitor_info_t local_fpga_monitor_inf  = {0};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

#include "fpga_monitor_info.h"
#include "platform.h"

void fpga_monitor_info_init(void)
{
    memset(&all_fpga_monitor_inf, 0, sizeof(all_fpga_monitor_inf));
}

void fpga_monitor_info_update(train_header_rx_t *header)
{
    fpga_monitor_inf_t *fpga_monitor_inf = &all_fpga_monitor_inf;

    uint8_t slot_id = header->fbk_slot_id;
    uint8_t port_id = header->fbk_port_id;
    uint16_t data_id = header->fbk_data[1];
    data_id = (data_id << 8) | header->fbk_data[0];

    uint32_t err_cnt = header->fbk_data[2];
    uint16_t pwr = header->fbk_data[4];
    pwr = (pwr << 8) | header->fbk_data[3];

    uint8_t port_idx = 0;

    // 与端口无关的监视信息
    if (slot_id < 4)
    {
        if (port_id == 0xFF)
        {
            if(data_id >= 1 && data_id <= MAX_FPGA_GEN_MONITOR_NUMBER)
            {
                uint8_t idx = (data_id - 1) / 2;
                fpga_monitor_inf->fpga_gen_monitor[slot_id][idx].slot_id = slot_id;
                if(data_id % 2 == 1)
                {
                    fpga_monitor_inf->fpga_gen_monitor[slot_id][idx].fpga_err_code = err_cnt;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_gen_info[idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_gen_info[idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_gen_monitor[slot_id][idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_gen_info[idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_gen_monitor[slot_id][idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_gen_info[idx].old_fpga_err_cnt;
                }
            }
            else if(data_id >= 500 && data_id <= 564)
            {
                uint8_t idx = data_id - 500 + MAX_FPGA_GEN_MONITOR_NUMBER;
                fpga_monitor_inf->fpga_gen_monitor[slot_id][idx].src_module_pwr = pwr;
            }
        }
        else
        {
            float module_pwr;
            if (pwr == 0xFFFF || pwr == 0xff)
            {
                module_pwr = -100;
            }
            else
            {
                module_pwr = (float)(log10((pwr) * 0.0001) * 10);
            }

            // 与端口相关的监视信息
            uint8_t is_status_update = (data_id % 2 == 1);
            // 区分端口类型
            switch((data_id - 1000) / 100)
            {
            case 0: // 百兆端口 1001-1064
                port_idx = (data_id - 1001) / 2;
                fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].slot_id = slot_id;
                if(is_status_update)
                {
                    fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].fpga_err_code = err_cnt;
                    fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].src_module_pwr = pwr;
                    fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].module_pwr_status = module_pwr;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_Meth_info[port_idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_Meth_info[port_idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_Meth_info[port_idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_Meth_monitor[slot_id][port_idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_Meth_info[port_idx].old_fpga_err_cnt;
                }
                break;
            case 1: // 千兆端口 1101-1164
                port_idx = (data_id - 1101) / 2;
                fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].slot_id = slot_id;
                if(is_status_update)
                {
                    fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].fpga_err_code = err_cnt;
                    fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].src_module_pwr = pwr;
                    fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].module_pwr_status = module_pwr;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_Geth_info[port_idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_Geth_info[port_idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_Geth_info[port_idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_Geth_monitor[slot_id][port_idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_Geth_info[port_idx].old_fpga_err_cnt;
                }
                break;
            case 2: // Aurora端口 1201-1264
                port_idx = (data_id - 1201) / 2;
                fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].slot_id = slot_id;
                if(is_status_update)
                {
                    fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].fpga_err_code = err_cnt;
                    fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].src_module_pwr = pwr;
                    fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].module_pwr_status = module_pwr;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_aurora_info[port_idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_aurora_info[port_idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_aurora_info[port_idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_aurora_monitor[slot_id][port_idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_aurora_info[port_idx].old_fpga_err_cnt;
                }
                break;
            case 3: // 扩展脉冲箱端口 1301-1364
                port_idx = (data_id - 1301) / 2;
                fpga_monitor_inf->fpga_sw_monitor[port_idx].slot_id = slot_id;
                if(is_status_update)
                {
                    fpga_monitor_inf->fpga_sw_monitor[port_idx].fpga_err_code = err_cnt;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_sw_info[port_idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_sw_info[port_idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_sw_monitor[port_idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_sw_info[port_idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_sw_monitor[port_idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_sw_info[port_idx].old_fpga_err_cnt;
                }
                break;
            case 4: // FT3端口 1401-1464
                port_idx = (data_id - 1401) / 2;
                fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].slot_id = slot_id;
                if(is_status_update)
                {
                    fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].fpga_err_code = err_cnt;
                    fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].src_module_pwr = pwr;
                    fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].module_pwr_status = module_pwr;
                }
                else
                {
                    uint8_t delta = (uint8_t)err_cnt - (uint8_t)local_fpga_monitor_inf.local_ft3_info[port_idx].old_fpga_err_cnt;
                    local_fpga_monitor_inf.local_ft3_info[port_idx].fpga_err_cnt_delta = delta;
                    fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].fpga_err_cnt += delta;
                    local_fpga_monitor_inf.local_ft3_info[port_idx].old_fpga_err_cnt = err_cnt;
                    fpga_monitor_inf->fpga_ft3_monitor[slot_id][port_idx].old_fpga_err_cnt = local_fpga_monitor_inf.local_ft3_info[port_idx].old_fpga_err_cnt;
                }
                break;
            }
        }
    }

    cache_wb_com(&all_fpga_monitor_inf, sizeof(fpga_monitor_inf_t), CacheP_TYPE_ALL);
}
