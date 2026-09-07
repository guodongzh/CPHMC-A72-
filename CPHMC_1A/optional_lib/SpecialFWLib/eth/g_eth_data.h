/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       eth_data.h
 *@author     LiuRui
 *@date       2025.09.23
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.23  1.0       LiuRui
 ******************************************************************************/

#ifndef _G_ETH_DATA_H
#define _G_ETH_DATA_H

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
#define GETH_FRAME_HEAD   (0x05640564)
#define GETH_FRAME_TAIL   (0x7E7E7E7E)
#define GETH_MAX_LEN      (32) // unit is word

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct _eth_addr
{
    uint8_t mac_addr[6];
} eth_addr_t;

typedef struct _eth_protocol
{
    eth_addr_t dst_mac;
    eth_addr_t src_mac;
    uint16_t eth_type;
    uint16_t appid;
    uint16_t eth_overhead;
    uint16_t comm_ver;
    uint8_t resv1[12];
} __PACKED eth_protocol_t;

typedef struct _eth_app_head
{
    uint32_t head;  // 0x05640564
    uint32_t timetag;
    uint16_t dat_num;  // 可以取8,16,24,32
    uint16_t frm_idx;
} __PACKED eth_app_head_t;

typedef struct _eth_app_tail
{
    uint32_t check_sum;
    uint32_t tail; // 0x7E7E7E7E
} __PACKED eth_app_tail_t;

typedef struct _eth_data
{
    eth_protocol_t proto_head;
    eth_app_head_t app_head;
    uint16_t app_data[];
} __PACKED eth_data_t;

typedef struct _eth_app_head_2
{
    uint16_t report_head;  // 0x0564
    uint16_t len;
    uint8_t app_frm_type;
    uint8_t tag_num;
    uint16_t delay_tine;
    uint16_t st_num;
    uint16_t sq_num;
    uint32_t frm_index;
    uint32_t sta_word;
} __PACKED eth_app_head_2_t;

typedef struct _eth_data_2
{
    eth_protocol_t proto_head;
    eth_app_head_2_t app_head;
    uint8_t app_data[];
} __PACKED eth_data_2_t;

typedef struct _yc_data
{
    uint8_t yc_tag; //0xa1 or 0xa2
    uint8_t yc_data_num;
    uint16_t frm_indx;
    uint32_t chan_vld[4];
    int32_t ana_data[];
} __PACKED yc_data_t;

typedef struct _geth_diag_info
{
    diag_info_t comm_diag;
    uint32_t dat_num_errs;
    uint32_t tail_errs;
    uint32_t yc_tag_errs;
    uint32_t disconnected;
} geth_diag_info_t;


typedef struct _pcie_geth_fbk
{
    car_rx_t *car[GETH_PORT_NUM];
    volatile uint16_t recv_frame_cnt[GETH_PORT_NUM];
    geth_diag_info_t diag_info[GETH_PORT_NUM];
    volatile uint16_t send_frame_cnt[GETH_PORT_NUM];
} pcie_geth_fbk_t;

extern pcie_geth_fbk_t g_pcie_geth_fbk;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void geth_send_data(uint8_t sfp_num, const uint16_t *app_data, eth_app_head_t *head);
void geth_recv_handler(train_rx_t *train_rx, uint8_t src_slot, car_rx_t *car);
eth_data_t *geth_recv_data(uint8_t sfp_num, diag_info_t **_diag_info);

#ifdef __cplusplus
}
#endif

#endif  //_G_ETH_DATA_H
