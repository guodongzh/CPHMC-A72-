/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       enet_queue_manage_core0.h
 *@author     wenjunf
 *@date       2025.02.17
 *@brief      以太网核间通信底层管理核接口
 *@par        History
 *Date        Version   Author     Description
 *2025.02.17  1.0       wenjunf    以太网核间通信底层管理核接口
 ******************************************************************************/
#ifndef _ENET_IPC_H
#define _ENET_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
#include "pcie_fpga.h"
#include "ipc_common.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define __PACKED           __attribute__((packed))

// #define A53_DIR            3

// #define S_CAR_MAX_NUM      2
// #define B_CAR_MAX_NUM      32
// #define RX_CAR_MAX_NUM     32

#define ENET_TX_MAX_NUM    0x7F

// FPGA->CPU:列车最大车厢数量(一节车厢代表一帧以太网报文)，该值由FPGA决定，经过调试后确定该值
#define PCIE_MAX_CAR_NUM   16

#define TX_PLAYLOAD_LEN    (2012U)
#define RX_PLAYLOAD_LEN    (2016U)

#define CAR_LEN_S          128   // car max length
#define CAR_LEN_B          2048  // car max length

#define MULTICAST_MAC_HEAD (0x10)
#define INET_PORT_TYPE     (0x41)      // 内网报文端口类型
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct enet_car_tx
{
    uint32_t ipc_frame_cnt;     // 核间通信帧计数
    uint16_t fream_head;        // 帧头: 0x1234：CPU->FPGA
    uint16_t port_frame_cnt;    // 对应端口帧计数
    uint8_t  msg_type;          // 报文类型：0x41-内网 0x02-外网
    uint8_t  send_mode1 : 4;    // 发送模式1 (0:中断沿发送; 1:立即发送; 2-15:其他)
    uint8_t  send_mode2 : 4;    // 发送模式2
    uint16_t length;            // payload有效数据长度
    uint32_t dstport_en_slot1;  // 槽位1目的端口标识
    uint32_t dstport_en_slot2;  // 槽位2目的端口标识
    uint32_t dstport_en_slot3;  // 槽位3目的端口标识
    uint32_t dstport_en_slot4;  // 槽位4目的端口标识
    uint16_t irq_num : 4;       // 中断号
    uint16_t resv1 : 12;        // 保留
    uint8_t  frame_pri;         // 报文优先级(1:高优先级,0:低优先级)
    uint8_t  resv4[5];          // 保留
    // 校验和从fream_head开始
    uint8_t payload[TX_PLAYLOAD_LEN];  // 标准以太网帧范围：(60~1514)字节,不包括FCS
} __PACKED enet_car_tx_t;

typedef struct enet_car_rx
{
    uint16_t fream_head;                // 帧头: 0x4321：FPGA->CPU
    uint16_t port_frame_cnt;            // 对应端口帧计数
    uint8_t  src_slot;                  // 槽位
    uint8_t  src_port;                  // 端口号
    uint8_t  port_type;                 // 端口类型
    uint8_t  resv1;                     // 保留
    uint16_t length;                    // payload有效数据长度
    uint32_t time_stamp;                // 数据帧时间戳，刻度8ns
    uint8_t  resv2[2];                  // 保留
    uint16_t ethtype;                   // 以太网类型
    uint16_t appid;                     // 装置地址
    uint16_t net_storm_state;           // 网络风暴状态信息
    uint8_t  dstmacaddr[6];             // 以太网目的地址
    uint8_t  resv3[4];                  // 保留
    uint8_t  payload[RX_PLAYLOAD_LEN];  // 标准以太网帧范围：(60~1514)字节,不包括FCS
} __PACKED enet_car_rx_t;

typedef struct _pcie_enet_tx_queue
{
    enet_car_tx_t     packets[ENET_TX_MAX_NUM + 1];
    volatile uint16_t length[ENET_TX_MAX_NUM + 1];
    volatile uint8_t  rd_point;
    volatile uint8_t  wr_point;
} pcie_enet_tx_queue_t;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern train_tx_t   *enet_tx_cfg;
extern enet_car_rx_t enet_rx_shm_que[RX_CAR_MAX_NUM];

extern enet_car_tx_t r1_tx_r0_que[2][S_CAR_MAX_NUM];
extern enet_car_tx_t r2_tx_r0_que[2][S_CAR_MAX_NUM];
extern enet_car_tx_t r3_tx_r0_que[2][S_CAR_MAX_NUM];
extern enet_car_tx_t a_tx_r0_que[B_CAR_MAX_NUM];

extern uint8_t       inet_macaddr[INET_CORE_NUM][6];

void ipc_scan_all_core_enet_txque_to_pcie_txque(void);
void push_enet_frm_to_pcie(void);
#ifdef CORE_R5F0
void pcie_to_enet_rxque(uint8_t *car_addr, uint8_t car_idx, uint8_t effect_car_num);
void ipc_scan_core0_enet_txque_to_pcie_txque(enet_car_tx_t *p_frame_src);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _ENET_IPC_H */