/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       aurora_data.h
 *@author     Xuquanbing
 *@date       2025.12.04
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.12.04  1.0       Xuquanbing
 ******************************************************************************/

#ifndef _AURORA_DATA_H
#define _AURORA_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "pcie_fpga.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define AURORA_MAX_BLK_NUM  16U
#define AURORA_BLK_DATA_NUM 8U

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct _aurora_data
{
    uint32_t data[AURORA_BLK_DATA_NUM];
} __attribute__((packed)) aurora_data_t;

typedef struct _std_aurora_frm
{
    aurora_data_t blk_data[AURORA_MAX_BLK_NUM];
} __attribute__((packed)) aurora_frm_t;

typedef struct _aurora_port_bit_map
{
    uint32_t port_bit_map[AURORA_PORT_NUM];
    uint8_t data_num[AURORA_PORT_NUM];
} aurora_send_cfg_t;

typedef struct _pcie_aurora_fbk
{
    uint8_t *payload[AURORA_PORT_NUM];
    uint8_t len[AURORA_PORT_NUM];
    diag_info_t diag_info[AURORA_PORT_NUM];
    uint8_t diag_process_flag[AURORA_PORT_NUM];
} pcie_aurora_fbk_t;

typedef struct _pcie_aurora_data
{
    uint8_t *payload[AURORA_PORT_NUM];
    uint32_t car_head_sum[AURORA_PORT_NUM];
    volatile uint16_t port_frame_cnt[AURORA_PORT_NUM];
} pcie_aurora_data_t;


extern pcie_aurora_fbk_t pcie_aurora_fbk;
extern aurora_send_cfg_t aurora_send_cfg;
extern pcie_aurora_data_t pcie_aurora_data;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void aurora_cfg(uint8_t slot_id, uint8_t sfp_id, uint8_t data_num);
void aurora_send_data(uint8_t slot_id,
                      uint8_t sfp_id,
                      uint8_t blk_num,
                      uint32_t data1,
                      uint32_t data2,
                      uint32_t data3,
                      uint32_t data4,
                      uint32_t data5,
                      uint32_t data6,
                      uint32_t data7,
                      uint32_t data8);
void aurora_build_tx_car(train_tx_t *tx_cfg, uint8_t slot_id, pcie_aurora_data_t *pPcieCmdPtr);
void aurora_calc_checksum(pcie_aurora_data_t *pPcieCmdPtr, uint8_t slot_id);
void aurora_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car);
aurora_data_t *aurora_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t blk_num);
void reset_aurora_diag_processing_flags(void);
diag_info_t *get_aurora_diag_info(uint8_t slot_id, uint8_t sfp_id);
#ifdef __cplusplus
}
#endif

#endif  //_AURORA_DATA_H
