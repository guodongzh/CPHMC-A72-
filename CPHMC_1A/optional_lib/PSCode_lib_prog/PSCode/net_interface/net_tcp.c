/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       net_tcp.c
*@author     LiuRui
*@date       2025.07.02
*@brief      smart plc net interface
*@par        History
*Date        Version   Author     Description
*2025.07.02  1.0       LiuRui     first version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "net_tcp.h"
#include "pscPlatform.h"
#include "pscWatch.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

// 可视化下位机和上位机通信时，数据采用一问一答的方式，所以只需要定义一个变量用于单独记录可视化相关的UDP数据信息
PSCode_UDP_INFO g_pscode_udp_info = {0};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
#ifdef BUILD_MCU2_0
#include "tcpip.h"
#include "net_private.h"

/**
 * @brief Receive data from the client
 * @return  received result
 */
uint32_t NET_PortRecv()
{
    uint32_t len = 0;

    // 当UDP接收队列中有数据，并且端口号是5000，则认为数据来自可视化
    if ((udp_recv_que.r_pointer != udp_recv_que.w_pointer) &&
        ((udp_recv_que.udp_data_info[udp_recv_que.r_pointer].lcl_port >= PORT_ID_BASE) &&
         (udp_recv_que.udp_data_info[udp_recv_que.r_pointer].lcl_port < PORT_ID_BASE + PSC_CORE_NUM)))
    {
        len = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].len;
        memcpy(gOpenMPC_RecvBuf, udp_recv_que.udp_data_info[udp_recv_que.r_pointer].buf, len);
        g_pscode_udp_info.pscode_dst_ip   = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].rmt_ipaddr;
        g_pscode_udp_info.pscode_dst_port = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].rmt_port;
        g_pscode_udp_info.pscode_src_port = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].lcl_port;
        g_pscode_udp_info.PortType        = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].PortType;
        g_pscode_udp_info.Src_Slot_ID     = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].Src_Slot_ID;
        g_pscode_udp_info.Src_Port_ID     = udp_recv_que.udp_data_info[udp_recv_que.r_pointer].Src_Port_ID;
        memcpy(&g_pscode_udp_info.pscode_dst_mac[0], &udp_recv_que.udp_data_info[udp_recv_que.r_pointer].rmtMac[0], 6);
        udp_recv_que.r_pointer            = (udp_recv_que.r_pointer + 1) % UDP_RECV_MAX_NUM;
    }

    return len;
}

/**
 * @brief Send data to the client
 * @param pbuf buffer pointer
 * @param size buffer size
 * @return  send result
 */
uint32_t NET_PortSend(uint8_t *pbuf, size_t size)
{
    udp_socket_t udp_socket;

    udp_socket.rmt_ipaddr  = g_pscode_udp_info.pscode_dst_ip;
    udp_socket.rmt_port    = g_pscode_udp_info.pscode_dst_port;
    udp_socket.lcl_port    = g_pscode_udp_info.pscode_src_port;
    udp_socket.PortType    = g_pscode_udp_info.PortType;
    udp_socket.Src_Slot_ID = g_pscode_udp_info.Src_Slot_ID;
    udp_socket.Src_Port_ID = g_pscode_udp_info.Src_Port_ID;
    memcpy(&udp_socket.rmtMac[0], &g_pscode_udp_info.pscode_dst_mac[0], 6);

    return udp_send(pbuf, size, &udp_socket);
}

/**
 * @brief Send wave data to the client
 * @param pbuf buffer pointer
 * @param size buffer size
 */
void PscWaveSend(uint8_t *pbuf, size_t size, uint16_t prot)
{
    udp_socket_t udp_socket;

    udp_socket.rmt_ipaddr  = g_pscode_udp_info.pscode_dst_ip;
    udp_socket.rmt_port    = prot;
    udp_socket.lcl_port    = g_pscode_udp_info.pscode_src_port;
    udp_socket.PortType    = g_pscode_udp_info.PortType;
    udp_socket.Src_Slot_ID = g_pscode_udp_info.Src_Slot_ID;
    udp_socket.Src_Port_ID = g_pscode_udp_info.Src_Port_ID;
    memcpy(&udp_socket.rmtMac[0], &g_pscode_udp_info.pscode_dst_mac[0], 6);

    udp_send(pbuf, size, &udp_socket);
}

#else

uint32_t NET_PortRecv()
{
        return 0;
}

uint32_t NET_PortSend(uint8_t *pbuf, size_t size)
{
    return 0;
}

void PscWaveSend(uint8_t *pbuf, size_t size, uint16_t prot)
{

}
#endif

