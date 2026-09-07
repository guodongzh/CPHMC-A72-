/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_port.h
*@author     xuesen
*@date       2026.07.08
*@brief      网络协议栈平台适配接口定义。
*@par        History
*Date        Version   Author     Description
*2026.07.08  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_PORT_H__
#define __NET_PORT_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include "net_other.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#ifndef ENET_MAC_ADDR_LEN
#define ENET_MAC_ADDR_LEN 6
#endif

#ifndef METH_UDP_IP_SOCKET_ID
#define METH_UDP_IP_SOCKET_ID 0
#endif

#ifndef NET_PRIMARY_CORE_ID
#define NET_PRIMARY_CORE_ID 0U
#endif

#ifndef MAX_ENET_SLOT_NUMBER
#define MAX_ENET_SLOT_NUMBER 4
#endif

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef enum
{
    NET_PLATFORM_G31RD = 0,
    NET_PLATFORM_C6X0,
    NET_PLATFORM_AM64X,
    NET_PLATFORM_J721E,
} net_platform_id_t;

// 协议层发送路由，记录报文来源端口及各槽位发送网口掩码
typedef struct
{
    uint32_t port_type;
    uint32_t slot_id;
    uint32_t port_id;
    uint16_t eth_mask[4];
} net_send_route_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 获取当前协议栈所在核心标识。
 * @return 当前核心标识
 */
uint32_t net_port_get_current_core_id(void);

/**
 * @brief 获取当前装置槽位号。
 * @return 当前槽位号
 */
uint32_t net_port_get_current_slot_id(void);

/**
 * @brief 获取用于计算通信标识的当前核心号。
 * @return 当前核心号
 */
uint32_t net_port_get_current_core_nr(void);

/**
 * @brief 获取当前装置板号。
 * @return 当前板号
 */
uint32_t net_port_get_board_no(void);

/**
 * @brief 登记平台提供的本机MAC和IP地址。
 * @param mac 本机MAC地址
 * @param ip 本机IP地址，使用协议栈内部字节序
 * @return true表示登记成功，false表示参数无效
 */
bool net_port_set_local_addr(const uint8_t mac[ENET_MAC_ADDR_LEN], ipaddr_t ip);

/**
 * @brief 获取平台登记的本机MAC和IP地址。
 * @param mac 输出的本机MAC地址
 * @param ip 输出的本机IP地址
 * @return true表示地址有效，false表示尚未登记或参数无效
 */
bool net_port_get_local_addr(ethaddr_t *mac, ipaddr_t *ip);

/**
 * @brief 根据接收报文更新本地网口的MAC和IP信息。
 * @param pInfo 接收报文及来源端口信息
 * @return true表示更新成功，false表示参数或配置无效
 */
bool net_port_get_current_enet_info(NET_RAW_PKG_INFO *pInfo);

/**
 * @brief 根据接收报文更新组网网口的MAC和IP信息。
 * @param pInfo 接收报文及来源端口信息
 * @return true表示更新成功，false表示参数或配置无效
 */
bool net_port_get_networking_enet_info(NET_RAW_PKG_INFO *pInfo);

/**
 * @brief 根据接收报文来源生成回包路由。
 * @param pInfo 接收报文及来源端口信息
 * @param pRoute 输出的发送路由
 */
void net_port_make_route_from_packet(const NET_RAW_PKG_INFO *pInfo, net_send_route_t *pRoute);

/**
 * @brief 通过普通网口发送以太网协议载荷。
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
                          const net_send_route_t *pRoute);

/**
 * @brief 通过组网网口发送以太网协议载荷。
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
                                 const net_send_route_t *pRoute);

/**
 * @brief 输出协议栈调试日志。
 * @param fmt 格式化字符串
 */
void net_port_debug_log(const char *fmt, ...);

/**
 * @brief 执行平台遗留网络初始化钩子。
 */
void inet_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PORT_H__ */
