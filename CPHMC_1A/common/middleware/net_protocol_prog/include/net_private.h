/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_private.h
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议栈内部常量与接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef _NET_PRIVATE_H_
#define _NET_PRIVATE_H_ 1

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

#define CFG_ETHTYPE_ARP  0x0806
#define CFG_ETHTYPE_IP   0x0800
#define CFG_ETHTYPE_IP6  0x86dd

#define CFG_PROTO_ICMP   1
#define CFG_PROTO_TCP    6
#define CFG_PROTO_UDP    17
#define CFG_PROTO_ICMP6  58

#define ICMP_ECHO_REPLY     0
#define ICMP_ECHO_REQUEST   8

#define CFG_NET_TTL      128

#define NET_ETH_HEAD_LEN (14)

#define CFG_ETHH_LEN     (14)
// IP头部长度
#if defined(CONFIG_NET_IPV6)
#define CFG_IPH_LEN 40
#else
#define CFG_IPH_LEN (20)
#endif

#define CFG_MULTIIP_NUM (3)

#define CFG_ICMPH_LEN       4

#define ICMP_ECHO_REPLY_LEN (4)
#define ICMP_ECHO_REQ_LEN   (4)

#define ICMP_LEN(len)       (CFG_ICMPH_LEN + (len))
#define ICMP_CMD2IPLEN(len) (NET_IPLEN(ICMP_LEN(len)))
#define ICMP_CMD2LEN(len)   (NET_PKGLEN(ICMP_CMD2IPLEN(len)))
#define ICMP_CMD2SIZE(len)  (NET_LEN2SIZE(ICMP_CMD2LEN(len)))

#define ICMP_IP2CMDLEN(len) ((int)len - CFG_IPH_LEN - CFG_ICMPH_LEN)

#define ICMP_MINLEN         (ICMP_CMD2LEN(0))

#define ICMP_ID             0x0100

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/


extern uint16_t IdIP;

// 组播地址表

extern const ipaddr_t MultiIPTbl[CFG_MULTIIP_NUM];

/**
 * @brief 初始化socket表。
 */
void socket_init(void);

/**
 * @brief 获取已创建socket数量。
 * @return socket数量
 */
uint16_t socket_count(void);

/**
 * @brief 按远端地址查找匹配的socket。
 * @param from 远端地址信息
 * @return 匹配的socket指针，未找到返回NULL
 */
socket_t *sock_ipin(const sockaddr_t *from);

/**
 * @brief 绑定UDP socket到本地地址。
 * @param psock socket对象
 * @param plocaladdr 本地地址
 * @param ethphyno 逻辑网口号
 * @return RTN_OK表示成功，RTN_ERR表示失败
 */
int32_t udp_bind(socket_t *psock, const sockaddr_t *plocaladdr, uint16_t ethphyno);

/**
 * @brief 将UDP接收数据写入接收队列。
 * @param buf UDP数据起始地址
 * @param len UDP数据长度
 * @param udp_Socket UDP连接信息
 * @return 写入的数据长度
 */
uint32_t udp_recv(const bystrm *buf, uint32_t len, socket_t *udp_Socket);

/**
 * @brief 发送UDP数据。
 * @param buf UDP数据起始地址
 * @param len UDP数据长度
 * @param udp_Socket UDP连接信息
 * @return 发送的数据长度
 */
uint32_t udp_send(const bystrm *buf, uint32_t len, udp_socket_t *udp_Socket);

/**
 * @brief 处理接收到的UDP报文。
 * @param nEthNo 逻辑网口号
 * @param pInfo 原始报文信息
 */
void udp_process(uint32_t nEthNo, NET_RAW_PKG_INFO *pInfo);

/**
 * @brief 处理接收到的ICMP报文。
 * @param pInfo 原始报文信息
 * @param nEthNo 逻辑网口号
 */
void icmp_recv(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo);

/**
 * @brief 检查IP地址是否在组播表中。
 * @param ipaddr 待检查IP地址
 * @return true表示在表中，false表示不在表中
 */
bool muti_chklist(const ipaddr_t *ipaddr);

#ifdef __cplusplus
}
#endif

#endif /* _NET_PRIVATE_H_ */