/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       tcpip.h
*@author     xuesen
*@date       2026.05.06
*@brief      TCP/IP协议栈公共接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __TCPIP_H__INCLUDE__
#define __TCPIP_H__INCLUDE__ 1

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

/*------------------------------------------------------------
 * 通用宏定义
 *-----------------------------------------------------------*/


#define CFG_NET_MTU        (1518 - 14 - 4)  // MTU (最大传输单元) 不包含以太网头和CRC
#define CFG_NET_HARDOFFSET (1)

#ifndef WIN32
#define INADDR_ANY 0
#endif

#define all_ones_ipaddr      0xFFFFFFFF
#define all_zeroes_ipaddr    0x00000000

/*------------------------------------------------------------
 * IP 地址基础配置（静态）
 *-----------------------------------------------------------*/
#define CFG_IPBASE           (((uint32_t)192 << 24) + ((uint32_t)178 << 16) + ((uint32_t)111 << 8))
#define CFG_MULTI_IPBASE     (((uint32_t)236 << 24) + ((uint32_t)8 << 16) + ((uint32_t)6 << 8))

#define CFG_MANAGE_MULTIIP   (CFG_MULTI_IPBASE + 4)
#define CFG_MANAGE_PORT      (8802)

/*------------------------------------------------------------
 * IP 分片标志位
 *-----------------------------------------------------------*/
#define MAYFRAG_IP           (0x4000)
#define MOREFRAG_IP          (0x2000)

/*------------------------------------------------------------
 * UDP宏定义
 *-----------------------------------------------------------*/
#define CFG_UDPH_LEN         (8)  // UDP头长度（字节）

#define UDP_LEN(len)         (CFG_UDPH_LEN + (len))             // UDP总长度
#define UDP_CMD2IPLEN(len)   (NET_IPLEN(UDP_LEN(len)))          // IP有效载荷长度（含UDP）
#define UDP_FRAG2IPLEN(len)  (NET_IPLEN(len))                   // 分片长度
#define UDP_CMD2LEN(len)     (NET_PKGLEN(UDP_CMD2IPLEN(len)))   // 总包长度（含MAC）
#define UDP_FRAG2LEN(len)    (NET_PKGLEN(UDP_FRAG2IPLEN(len)))  // 分片包总长度
#define UDP_MINLEN           (UDP_CMD2IPLEN(0))
#define UDP_MAXLEN           (CFG_NET_MTU - CFG_IPH_LEN - CFG_UDPH_LEN)  // 这里没考虑VLAN
#define SNTP_PORT            123

#define ETH_HEADER_LEN       14  // 以太网头部长度
#define IP_HEADER_LEN        20  // IP头部长度
#define UDP_HEADER_LEN       8   // UDP头部长度

/*------------------------------------------------------------
 * 结构体字段位置索引（用于 uint32_t[] 格式的报文数组）
 *-----------------------------------------------------------*/
#define NET_IP_SRCIP_SUF     (7)
#define NET_UDP_SRCPORT_SUF  (9)
#define NET_UDP_DESTPORT_SUF (9)
#define NET_UDP_LEN_SUF      (10)
#define NET_UDPHEAD_SUF      (11)

/*------------------------------------------------------------
 * UDP 长度宏（以字节为单位）
 *-----------------------------------------------------------*/
#define UDP_RAWLEN8(len)     (NET_ETH_HEAD_LEN + NET_IP_HEAD_LEN + CFG_UDPH_LEN + (len))
#define UDP_FRAGLEN8(len)    (NET_ETH_HEAD_LEN + NET_IP_HEAD_LEN + (len))
#define UDP_CMD2SIZE(len)    (LEN8_TO_SIZE32(UDP_RAWLEN8(len)))

/*------------------------------------------------------------
 * 工具宏定义
 *-----------------------------------------------------------*/
#define ipaddr_cmp(addr1, addr2)           ((addr1) == (addr2))
#define ipaddr_maskcmp(addr1, addr2, mask) (((addr1) & (mask)) == ((addr2) & (mask)))
#define ipaddr_copy(dest, src)             ((dest) = (src))

#define muti_chkip(ip)                     (0xe0000000 == ((ip) & 0xF0000000))

#define NET_LEN2SIZE(len)                  (FIFO_PRE_DATA_SIZE32 + BYTE_TO_DWORD(CFG_NET_TX_OFFSET + (len)) + 1)
#define NET_IPLEN(len)                     (CFG_IPH_LEN + (len))
#define NET_PKGLEN(len)                    (CFG_ETHH_LEN + (len))

#define TOIPADDR(a, b, c, d)               (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*------------------------------------------------------------
 * 以太网设备统计信息结构
 *-----------------------------------------------------------*/
typedef struct
{


    uint32_t state;     // 当前工作状态
    uint32_t mode;      // 当前模式
    uint32_t rx_byte;   // 接收字节计数
    uint32_t tx_byte;   // 发送字节计数
    uint32_t rx_packet; // 接收包计数
    uint32_t tx_packet; // 发送包计数
} ethinfo_t;

/*------------------------------------------------------------
 * 网络驱动回调定义
 *-----------------------------------------------------------*/
typedef int32_t netput_t(uint32_t *buf, uint16_t len);       // 网络输出函数原型
typedef int32_t netget_t(const uint32_t *buf, uint16_t len); // 网络输入函数原型
typedef bool    net_driver_puttst_t(int32_t);
typedef int32_t net_driver_put_t(uint8_t ethNo, uint32_t *buf, int32_t len);

typedef struct net_driver_t
{
    net_driver_put_t *put; // 发送函数指针
} net_driver_t;

/*------------------------------------------------------------
 * 网络协议栈调试
 *-----------------------------------------------------------*/
typedef struct _net_protocol_debug
{
    uint32_t err_number;   // 报告存储出错次数
    uint32_t err_location; // 报告存储出错定位
} net_protocol_debug_t;

/*------------------------------------------------------------
 * 全局变量（在 net_xxx.c 中定义）
 *-----------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

extern ipaddr_t                            g_EthIpAddr[INET_MAC_COUNT];
extern ipaddr_t                            g_EthMask[INET_MAC_COUNT];
extern ipaddr_t                            g_EthGateway[INET_MAC_COUNT];
extern net_protocol_debug_t                net_protocol_debug;
extern udp_recv_que_t                      udp_recv_que;
/*------------------------------------------------------------
 * 函数声明
 *-----------------------------------------------------------*/
/**
 * @brief 处理接收到的以太网原始报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示处理失败
 */
bool tcpip_process(NET_RAW_PKG_INFO * pInfo);

/**
 * @brief 初始化网络协议栈。
 */
void protocol_stack_init(void);

/**
 * @brief 初始化 TCP/IP 协议栈。
 * @param pTxFifo 发送FIFO句柄，统一协议层不直接使用
 * @param pCallbackUdp UDP接收回调函数
 */
void tcpip_init(acp_buf_t *pTxFifo, callback_recvfrom_udp *pCallbackUdp);

/**
 * @brief 根据IP地址生成以太网MAC地址。
 * @param ethaddr 输出的MAC地址
 * @param ipaddr 输入的IP地址
 */
void iptoeth(ethaddr_t *ethaddr, const ipaddr_t *ipaddr);

/**
 * @brief 将点分十进制字符串转换为IP地址。
 * @param str IP地址字符串
 * @param ipaddr 输出的IP地址
 * @return true表示转换成功，false表示转换失败
 */
bool strtoip(const char *str, ipaddr_t *ipaddr);

/**
 * @brief 填充以太网帧头部的源MAC和目的MAC。
 * @param buf 以太网帧缓冲区
 * @param d_ethaddr 目的MAC地址
 * @param s_ethaddr 源MAC地址
 */
void eth_head(uint32_t *buf, const ethaddr_t *d_ethaddr, const ethaddr_t *s_ethaddr);

/**
 * @brief 累加TCP/IP校验和数据。
 * @param sum 初始累加值
 * @param buffer 数据缓冲区
 * @param bytes 数据长度
 * @return 累加后的校验和值
 */
uint32_t tcpip_checksum(uint32_t sum, const uint16_t *buffer, uint32_t bytes);

/**
 * @brief 计算网络报文校验和。
 * @param dp 报文数据起始地址
 * @param len 数据长度
 * @return 16位校验和
 */
uint16_t checksum_net(const uint32_t *dp, uint32_t len);

/**
 * @brief 计算UDP校验和。
 * @param eth_send_buff IP报文起始缓冲区
 * @param udp_len UDP报文长度
 * @return UDP校验和
 */
uint16_t udp_checksum(uint8_t *eth_send_buff, uint16_t udp_len);

/**
 * @brief 构造不分片的IPv4头部。
 * @param pRawData IP头部输出缓冲区
 * @param s_ipaddr 源IP地址
 * @param d_ipaddr 目的IP地址
 * @param len IP报文总长度
 * @param proto 上层协议号
 * @return 写入后的缓冲区指针
 */
uint32_t *ip_head(uint32_t *pRawData, const ipaddr_t *s_ipaddr, const ipaddr_t *d_ipaddr, uint16_t len, uint8_t proto);

/**
 * @brief 构造带分片偏移的IPv4头部。
 * @param pRawData IP头部输出缓冲区
 * @param s_ipaddr 源IP地址
 * @param d_ipaddr 目的IP地址
 * @param len IP报文总长度
 * @param ipoffset 分片标志和偏移
 * @param proto 上层协议号
 */
void ip_head2(uint32_t *      pRawData,
              const ipaddr_t *s_ipaddr,
              const ipaddr_t *d_ipaddr,
              uint16_t        len,
              uint16_t        ipoffset,
              uint8_t         proto);

/**
 * @brief 从接收报文中解析目的IP地址。
 * @param pInfo 原始报文信息
 * @param ipaddr 输出的目的IP地址
 */
void ip_to(NET_RAW_PKG_INFO * pInfo, ipaddr_t * ipaddr);

#ifdef __cplusplus
}
#endif

#endif /* __TCPIP_H__INCLUDE__ */
