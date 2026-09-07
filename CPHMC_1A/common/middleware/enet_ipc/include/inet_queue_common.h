/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       inet_queue_common.h
 *@author     jinyangh
 *@date       2025.11.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.11.25  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _INET_QUEUE_COMMON_H
#define _INET_QUEUE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "enet_queue_common.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define INET_TX_MODE_NUM               2        // 内网报文发送模式的数量（单播、组播）
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef enum {
    Unicast   = 0x00,
    Multicast = 0xFF,
} inet_tx_mode;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern uint8_t        inet_macaddr[INET_CORE_NUM][6];
extern enet_car_tx_t  r0_multicast_que[S_CAR_MAX_NUM] ENET_R0_MULTICAST_SHARE;
extern enet_car_tx_t  r0_tx_r1_que[S_CAR_MAX_NUM] ENET_R0_TX_R1_SHARE;
extern enet_car_tx_t  r2_tx_r1_que[S_CAR_MAX_NUM] ENET_R2_TX_R1_SHARE;
extern enet_car_tx_t  r3_tx_r1_que[S_CAR_MAX_NUM] ENET_R3_TX_R1_SHARE;
extern enet_car_tx_t  r1_multicast_que[S_CAR_MAX_NUM] ENET_R1_MULTICAST_SHARE;
extern enet_car_tx_t  r0_tx_r2_que[S_CAR_MAX_NUM] ENET_R0_TX_R2_SHARE;
extern enet_car_tx_t  r1_tx_r2_que[S_CAR_MAX_NUM] ENET_R1_TX_R2_SHARE;
extern enet_car_tx_t  r3_tx_r2_que[S_CAR_MAX_NUM] ENET_R3_TX_R2_SHARE;
extern enet_car_tx_t  r2_multicast_que[S_CAR_MAX_NUM] ENET_R2_MULTICAST_SHARE;
extern enet_car_tx_t  r0_tx_r3_que[S_CAR_MAX_NUM] ENET_R0_TX_R3_SHARE;
extern enet_car_tx_t  r1_tx_r3_que[S_CAR_MAX_NUM] ENET_R1_TX_R3_SHARE;
extern enet_car_tx_t  r2_tx_r3_que[S_CAR_MAX_NUM] ENET_R2_TX_R3_SHARE;
extern enet_car_tx_t  r3_multicast_que[S_CAR_MAX_NUM] ENET_R3_MULTICAST_SHARE;

void inet_init(void);
void rec_net_raw_core_2_core_port_enable(uint32_t rec_port);
bool inet_recv_to_rxfifo(void);
void inet_send_to_txque(enet_car_tx_t *p_frame_src);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _INET_QUEUE_COMMON_H */