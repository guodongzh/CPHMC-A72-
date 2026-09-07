/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_port_g31rd.c
*@author     xuesen
*@date       2026.07.08
*@brief      G31RD/通用平台网络协议适配实现。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#include "net_all_include.h"

#if defined(NET_PORT_G31RD) || (!defined(NET_PORT_AM64X) && !defined(NET_PORT_J721E) && !defined(NET_PORT_C6X0))

#include "eth_mac.h"
#include "fcpu_info.h"

#include <stdarg.h>
#include <stdio.h>

static ethaddr_t s_local_mac;
static ipaddr_t  s_local_ip;
static bool      s_local_addr_valid;

/**
 * @brief 获取当前G31RD处理器核心标识。
 * @return 当前核心标识
 */
uint32_t net_port_get_current_core_id(void)
{
    uint32_t cpu_id = 0;

    GetCpuId(&cpu_id);
    return cpu_id;
}

/**
 * @brief 获取G31RD装置槽位号。
 * @return 固定返回0
 */
uint32_t net_port_get_current_slot_id(void)
{
    return 0;
}

/**
 * @brief 获取用于计算通信标识的G31RD核心号。
 * @return 当前核心标识
 */
uint32_t net_port_get_current_core_nr(void)
{
    return net_port_get_current_core_id();
}

/**
 * @brief 获取当前G31RD装置板号。
 * @return 当前板号
 */
uint32_t net_port_get_board_no(void)
{
    return GetBoardNo();
}

/**
 * @brief 登记G31RD平台提供的本机MAC和IP地址。
 * @param mac 本机MAC地址
 * @param ip 本机IP地址
 * @return true表示登记成功，false表示参数无效
 */
bool net_port_set_local_addr(const uint8_t mac[ENET_MAC_ADDR_LEN], ipaddr_t ip)
{
    if ((mac == NULL) || (ip == 0U))
    {
        return false;
    }

    memcpy(s_local_mac.addr, mac, ENET_MAC_ADDR_LEN);
    s_local_ip         = ip;
    s_local_addr_valid = true;
    return true;
}

/**
 * @brief 获取G31RD平台登记的本机MAC和IP地址。
 * @param mac 输出的本机MAC地址
 * @param ip 输出的本机IP地址
 * @return true表示地址有效，false表示尚未登记或参数无效
 */
bool net_port_get_local_addr(ethaddr_t *mac, ipaddr_t *ip)
{
    if ((mac == NULL) || (ip == NULL) || !s_local_addr_valid)
    {
        return false;
    }

    *mac = s_local_mac;
    *ip  = s_local_ip;
    return true;
}

/**
 * @brief 加载G31RD本地网口的MAC和IP信息。
 * @param pInfo 接收报文信息，G31RD平台不使用
 * @return 固定返回true
 */
bool net_port_get_current_enet_info(NET_RAW_PKG_INFO *pInfo)
{
    (void)pInfo;
    return net_port_get_local_addr(&self_SrcMacAddr, &self_ipaddr);
}

/**
 * @brief 拒绝获取G31RD平台不存在的组网地址。
 * @param pInfo 接收报文信息，G31RD平台不使用
 * @return 固定返回false
 */
bool net_port_get_networking_enet_info(NET_RAW_PKG_INFO *pInfo)
{
    (void)pInfo;
    return false;
}

/**
 * @brief 根据接收报文来源生成统一回包路由。
 * @param pInfo 接收报文及来源端口信息
 * @param pRoute 输出的发送路由
 */
void net_port_make_route_from_packet(const NET_RAW_PKG_INFO *pInfo, net_send_route_t *pRoute)
{
    if (pRoute == NULL)
    {
        return;
    }
    memset(pRoute, 0, sizeof(*pRoute));
    if (pInfo != NULL)
    {
        pRoute->port_type = pInfo->PortType;
        pRoute->slot_id   = pInfo->Src_Slot_ID;
        pRoute->port_id   = pInfo->Src_Port_ID;
    }
    if (pRoute->slot_id < ARRAYSIZEOF(pRoute->eth_mask))
    {
        pRoute->eth_mask[pRoute->slot_id] = (uint16_t)(1U << (pRoute->port_id & 0xFU));
    }
}

/**
 * @brief 通过G31RD以太网驱动发送协议载荷。
 * @param frame 以太网类型字段后的协议载荷
 * @param len 协议载荷长度
 * @param dst_mac 目的MAC地址
 * @param eth_type 以太网类型
 * @param pRoute 发送路由，G31RD平台不使用
 * @return 大于0表示发送成功，负值表示发送失败
 */
int32_t net_port_send_raw(const uint8_t *frame,
                          uint32_t len,
                          const uint8_t dst_mac[ENET_MAC_ADDR_LEN],
                          uint16_t eth_type,
                          const net_send_route_t *pRoute)
{
    int32_t send_ret;

    (void)pRoute;
    send_ret = eth_mac_send(dst_mac, eth_type, frame, len);

    // G31RD驱动以0表示成功，统一转换为协议栈要求的已发送长度
    return (send_ret == 0) ? (int32_t)len : send_ret;
}

/**
 * @brief 拒绝通过G31RD平台不存在的组网通道发送协议载荷。
 * @param frame 以太网类型字段后的协议载荷
 * @param len 协议载荷长度
 * @param dst_mac 目的MAC地址
 * @param eth_type 以太网类型
 * @param pRoute 发送路由
 * @return 固定返回-1
 */
int32_t net_port_send_networking(const uint8_t *frame,
                                 uint32_t len,
                                 const uint8_t dst_mac[ENET_MAC_ADDR_LEN],
                                 uint16_t eth_type,
                                 const net_send_route_t *pRoute)
{
    (void)frame;
    (void)len;
    (void)dst_mac;
    (void)eth_type;
    (void)pRoute;
    return -1;
}

/**
 * @brief 输出G31RD协议栈调试日志。
 * @param fmt 格式化字符串
 */
void net_port_debug_log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#endif
