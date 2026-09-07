/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       net_tcp.h
 *@author     LiuRui
 *@date       2025.07.02
 *@brief      smart plc net interface
 *@par        History
 *Date        Version   Author     Description
 *2025.07.02  1.0       LiuRui     first version
 ******************************************************************************/

#ifndef NET_TCP_H
#define NET_TCP_H

#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint32_t pscode_dst_ip;     //用于记录可视化上位机的ip地址
    uint32_t pscode_dst_mac[6]; //用于记录可视化上位机的mac地址
    uint16_t pscode_dst_port;   //用于记录可视化上位机的端口号
    uint16_t pscode_src_port;
    uint32_t PortType;
    uint32_t Src_Slot_ID;
    uint32_t Src_Port_ID;
}PSCode_UDP_INFO; //用于单独记录可视化相关的UDP数据信息：因为协议栈可能一次性处理多包UDP数据，为了防止可视化的UDP数据信息被覆盖，所以单独记录

extern PSCode_UDP_INFO g_pscode_udp_info;
extern uint8_t         gOpenMPC_RecvBuf[5000];
extern uint8_t         gOpenMPC_SendBuf[5000];

uint32_t NET_PortRecv();
uint32_t NET_PortSend(uint8_t *pbuf, size_t size);
void     PscWaveSend(uint8_t *pbuf, size_t size, uint16_t prot);
#endif  // NET_TCP_H
