/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_ft3.h
 *@author     LiuRui
 *@date       2025.03.31
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.03.31  1.0       LiuRui
 ******************************************************************************/

#ifndef _FT3_DATA_H
#define _FT3_DATA_H

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include "pcie_fpga.h"
#include "pcie_spi_cfg.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define FT3_MAX_BLK_NUM  5
#define FT3_MAX_CH_NUM   4
#define FT3_BLK_DATA_NUM 8

#define CFG_INVALID      (0u)
#define CFG_VALID        (1u)
#define CFG_PASS         (2u)
#define CFG_FAIL         (3u)
#define ERR_CH_NUM       (4u)
#define ERR_BLK_NUM      (5u)
#define ERR_LAST_BLK_LEN (6u)

#define FT3_RX_CFG_FLAG  (0x5A)

// 包含CRC
#define STD_FT3_BLK_LEN  (18u)  // (16 data +  2 )

#define FT3_FRAME_HEAD   (0x0564)
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef struct _ft3_cfg_chk
{
    ft3_cfg_t cfg_data;
    ft3_cfg_t cfg_data_bak;
    uint32_t cfg_state;
} __attribute__((aligned(128))) ft3_cfg_chk_t;

typedef struct _ft3_data
{
    uint16_t data[FT3_BLK_DATA_NUM];  // 标准FT3每个Block为8个双字节数据
    uint16_t crc;
} __attribute__((packed)) ft3_data_t;

typedef struct _std_ft3_frm
{
    uint16_t ft3_head;  // 0x0564
    ft3_data_t blk_data[FT3_MAX_BLK_NUM];
} __attribute__((packed)) std_ft3_frm_t;

typedef struct _ext_ft3_frm
{
    uint16_t ft3_head;      // 0x0564
    uint16_t ext_data[45];  // TODO 包含crc,为可变结构,便于调试
} __attribute__((packed)) ext_ft3_frm_t;

typedef struct _ft3_blk_data
{
    ft3_data_t blk_data[FT3_MAX_BLK_NUM];
} __attribute__((packed)) ft3_blk_data_t;

typedef struct _ft3_tx_ctrl
{
    uint8_t port_num;                       // 配置的发送端口数量
    uint16_t car_len[FT3_TX_CH_NUM];      // 每个端口对应的发送车厢的16字节对齐后的长度
    uint16_t payload_len[FT3_TX_CH_NUM];  // 每个端口对应的发送车厢的有效数据长度
    uint8_t last_blk_len[FT3_RX_CH_NUM];
    uint8_t diff_car_len[FT3_TX_CH_NUM];  // 每个端口对应的发送车厢的16字节对齐所需补零的长度
    uint32_t ft3_ch_bit_map;
} ft3_tx_ctrl_t;

typedef struct _ft3_rx_ctrl
{
    uint8_t port_num;                       // TODO 配置的接收端口数量(debug使用,后续移除)
    uint8_t port_id[FT3_RX_CH_NUM];       // TODO 端口号(debug使用,后续移除)
    uint16_t car_len[FT3_RX_CH_NUM];      // 每个端口对应的发送车厢的16字节对齐后的长度(debug使用,后续移除)
    uint16_t payload_len[FT3_RX_CH_NUM];  // 每个端口对应的发送车厢的有效数据长度
    uint8_t last_blk_len[FT3_RX_CH_NUM];
    uint32_t ft3_ch_bit_map;
} ft3_rx_ctrl_t;

typedef struct _ft3_shm_cfg
{
    uint8_t cfg_state;
    ft3_rx_ctrl_t ft3_rx_ctrl[FT3_SOLT_NUM];
    ft3_tx_ctrl_t ft3_tx_ctrl[FT3_SOLT_NUM];
} ft3_shm_cfg_t;

typedef struct _ft3_sta
{
    uint16_t pptical_power;
    uint16_t frame_cnt;
    uint8_t err_cnt;
    uint8_t err_code;
} _ft3_sta_t;

typedef struct _pcie_ft3_fbk
{
    uint8_t *payload[FT3_SOLT_NUM][FT3_RX_CH_NUM];
    diag_info_t diag_info[FT3_SOLT_NUM][FT3_RX_CH_NUM];
    uint8_t diag_process_flag[FT3_SOLT_NUM][FT3_RX_CH_NUM];
} pcie_ft3_fbk_t;

typedef struct _pcie_cmd_ptr
{
    uint8_t *payload[FT3_SOLT_NUM][FT3_TX_CH_NUM];
    uint32_t car_head_sum[FT3_SOLT_NUM][FT3_TX_CH_NUM];
    volatile uint16_t port_frame_cnt[FT3_SOLT_NUM][FT3_TX_CH_NUM];
} pcie_ft3_data_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

extern ft3_tx_ctrl_t ft3_tx_ctrl[FT3_SOLT_NUM];
extern ft3_cfg_chk_t ft3_tx_cfg_all[FT3_SOLT_NUM][FT3_TX_CH_NUM];
extern ft3_cfg_chk_t ft3_rx_cfg_all[FT3_SOLT_NUM][FT3_RX_CH_NUM];
#ifndef BUILD_C7X_1
extern ft3_cfg_chk_t ft3_tx_cfg_all_prv[FT3_SOLT_NUM][FT3_TX_CH_NUM];
extern ft3_cfg_chk_t ft3_rx_cfg_all_prv[FT3_SOLT_NUM][FT3_RX_CH_NUM];
#endif

extern ft3_rx_ctrl_t ft3_rx_ctrl[FT3_SOLT_NUM];
extern pcie_ft3_fbk_t pcie_ft3_fbk;
extern pcie_ft3_data_t pcie_ft3_data;

void ft3_set_cfg(ft3_cfg_t *cfg_data,
                 uint8_t slot_id,
                 uint8_t sfp_id,
                 uint8_t port_id,
                 uint8_t is_tx);
uint8_t ft3_get_cfg(ft3_cfg_t *cfg_data,
                    uint8_t slot_id,
                    uint8_t sfp_id,
                    uint8_t port_id,
                    uint8_t is_tx);

void pcie_send_ft3_cfg(uint32_t slot_num);

void ft3_build_tx_car(train_tx_t *tx_cfg, uint8_t slot_num, pcie_ft3_data_t *pPcieCmdPtr);
void ft3_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car);
void ft3_calc_checksum(pcie_ft3_data_t *pPcieCmdPtr, uint8_t slot_num);

void ft3_send_data(uint8_t slot_id,
                   uint8_t sfp_id,
                   uint8_t port_num,
                   uint8_t blk_num,
                   uint16_t data1,
                   uint16_t data2,
                   uint16_t data3,
                   uint16_t data4,
                   uint16_t data5,
                   uint16_t data6,
                   uint16_t data7,
                   uint16_t data8);
uint16_t *extend_ft3_send_data(uint8_t slot_id, uint8_t sfp_id, uint8_t port_num, uint8_t *ext_data_len);

uint16_t *ft3_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id, uint8_t blk_num);
uint16_t *extend_ft3_recv_data(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id, uint8_t *valid_num);
void reset_ft3_diag_processing_flags(void);
diag_info_t *get_ft3_diag_info(uint8_t slot_id, uint8_t sfp_id, uint8_t port_id);
#endif
