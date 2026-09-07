/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_port_am64x.c
*@author     xuesen
*@date       2026.07.08
*@brief      AM64X 平台网络协议适配实现。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#include "net_all_include.h"

#if defined(NET_PORT_AM64X)

#include <stdarg.h>

#if defined(__has_include)
#if __has_include("can_ipc.h")
#include "can_ipc.h"
#endif
#if __has_include("platform.h")
#include "platform.h"
#endif
#if __has_include("wdg_monitor.h")
#include "wdg_monitor.h"
#endif
#endif

static CURRENT_CORE_ENET_MAC_IP_INF_STRUCT s_core_enet_inf;
static ethaddr_t                           s_local_mac;
static ipaddr_t                            s_local_ip;
static bool                                s_local_addr_valid;

/**
 * @brief 将统一发送路由转换为AM64x以太网驱动路由。
 * @param pRoute 统一发送路由
 * @param pPortDef 输出的驱动发送路由
 */
static void net_port_route_to_enet(const net_send_route_t *pRoute, ENET_SEND_PORT_DEF_STRUCT *pPortDef)
{
    memset(pPortDef, 0, sizeof(*pPortDef));
    if (pRoute != NULL)
    {
        pPortDef->enet_send_Port_type = pRoute->port_type;
        for (uint32_t i = 0; i < MAX_ENET_SLOT_NUMBER && i < ARRAYSIZEOF(pRoute->eth_mask); i++)
        {
            pPortDef->nEthMask[i] = pRoute->eth_mask[i];
        }
    }
}

/**
 * @brief 获取当前AM64x核心标识。
 * @return 当前核心标识
 */
uint32_t net_port_get_current_core_id(void)
{
    return get_Current_Core_ID();
}

/**
 * @brief 获取当前AM64x装置槽位号。
 * @return 当前槽位号
 */
uint32_t net_port_get_current_slot_id(void)
{
    return get_Current_Slot_ID();
}

/**
 * @brief 获取用于计算通信标识的AM64x核心号。
 * @return 当前核心号
 */
uint32_t net_port_get_current_core_nr(void)
{
    return get_Current_Core_ID();
}

/**
 * @brief 获取当前AM64x装置板号。
 * @return 当前板号
 */
uint32_t net_port_get_board_no(void)
{
    return GetBoardNo();
}

/**
 * @brief 登记AM64X平台当前选中的本机MAC和IP地址。
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
 * @brief 获取AM64X平台最近登记的本机MAC和IP地址。
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
 * @brief 根据接收端口加载AM64x本地网口的MAC和IP信息。
 * @param pInfo 接收报文及来源端口信息
 * @return true表示加载成功，false表示参数或配置无效
 */
bool net_port_get_current_enet_info(NET_RAW_PKG_INFO *pInfo)
{
    CURRENT_ENET_INF_STRUCT current_inf = {0};

    if (pInfo == NULL)
    {
        return false;
    }

    current_inf.Net_type = pInfo->PortType;
    current_inf.Core_ID  = get_Current_Core_ID();
    current_inf.Slot_ID  = get_Current_Slot_ID();
    current_inf.Net_ID   = pInfo->Src_Port_ID / 2;
    if (!get_locate_enet_inf(&s_core_enet_inf, current_inf))
    {
        return false;
    }

    memcpy(&self_SrcMacAddr, s_core_enet_inf.srcMac, sizeof(ethaddr_t));
    memcpy(&self_ipaddr, s_core_enet_inf.src_IP, sizeof(ipaddr_t));
    return true;
}

/**
 * @brief 根据接收端口加载AM64x组网网口的MAC和IP信息。
 * @param pInfo 接收报文及来源端口信息
 * @return true表示加载成功，false表示参数或配置无效
 */
bool net_port_get_networking_enet_info(NET_RAW_PKG_INFO *pInfo)
{
    CURRENT_ENET_INF_STRUCT current_inf = {0};

    if (pInfo == NULL)
    {
        return false;
    }

    current_inf.Net_type = pInfo->PortType;
    current_inf.Core_ID  = get_Current_Core_ID();
    current_inf.Slot_ID  = get_Current_Slot_ID();
    current_inf.Net_ID   = pInfo->Src_Port_ID / 2;
    if (!get_net_config_info(&s_core_enet_inf, current_inf))
    {
        return false;
    }

    memcpy(&net_SrcMacAddr, s_core_enet_inf.srcMac, sizeof(ethaddr_t));
    memcpy(&net_ipaddr, s_core_enet_inf.src_IP, sizeof(ipaddr_t));
    return true;
}

/**
 * @brief 根据接收报文来源生成AM64x回包路由。
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
 * @brief 通过AM64x普通网口发送以太网协议载荷。
 * @param frame 以太网类型字段后的协议载荷
 * @param len 协议载荷长度
 * @param dst_mac 目的MAC地址
 * @param eth_type 以太网类型，使用主机字节序
 * @param pRoute 发送路由
 * @return 大于0表示发送成功，其余值表示发送失败
 */
int32_t net_port_send_raw(const uint8_t *frame,
                          uint32_t len,
                          const uint8_t dst_mac[ENET_MAC_ADDR_LEN],
                          uint16_t eth_type,
                          const net_send_route_t *pRoute)
{
    ENET_SEND_PORT_DEF_STRUCT port_def;

    net_port_route_to_enet(pRoute, &port_def);
    return sendto_raw_socket(0U, (uint32_t)METH_UDP_IP_SOCKET_ID, frame, len, 0, port_def, dst_mac, htons(eth_type), 0U);
}

/**
 * @brief 通过AM64x组网网口发送以太网协议载荷。
 * @param frame 以太网类型字段后的协议载荷
 * @param len 协议载荷长度
 * @param dst_mac 目的MAC地址
 * @param eth_type 以太网类型，使用主机字节序
 * @param pRoute 发送路由
 * @return 大于0表示发送成功，其余值表示发送失败
 */
int32_t net_port_send_networking(const uint8_t *frame,
                                 uint32_t len,
                                 const uint8_t dst_mac[ENET_MAC_ADDR_LEN],
                                 uint16_t eth_type,
                                 const net_send_route_t *pRoute)
{
    ENET_SEND_PORT_DEF_STRUCT port_def;

    net_port_route_to_enet(pRoute, &port_def);
    return sendto_networking_socket(0U, (uint32_t)METH_UDP_IP_SOCKET_ID, frame, len, 0, port_def, dst_mac, htons(eth_type), 0U);
}

/**
 * @brief 保留AM64x协议栈调试日志接口。
 * @param fmt 格式化字符串
 */
void net_port_debug_log(const char *fmt, ...)
{
    (void)fmt;
}

#endif
