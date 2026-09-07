/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_arp.h
*@author     xuesen
*@date       2026.05.06
*@brief      ARP报文处理与缓存表管理接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_ARP_H__INCLUDE__
#define __NET_ARP_H__INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


#define MAX_SLOT_NUM    (4)
#define MAX_PORT_NUM    (32)
#define MAX_ARPTAB_NUM  (32)
// #define CFG_ARPTAB_SIZE (32)

#define ARP_REQUEST     (1)
#define ARP_REPLY       (2)

#define ARP_HWTYPE_ETH  (1)

#define CFG_ETHTYPE_ARP 0x0806


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef struct arp_entry_t
{
    ipaddr_t  ipaddr;      // IP地址
    ethaddr_t ethaddr;     // MAC地址
    uint32_t  PortType;    // 端口类型
    uint32_t  Src_Slot_ID; // 槽位序号
    uint32_t  Src_Port_ID; // 端口序号
} arp_entry_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 初始化 ARP 缓存表。
 */
void arp_init(void);

/**
 * @brief 处理接收到的 ARP 报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示报文无效
 */
int32_t arp_recv(NET_RAW_PKG_INFO * pInfo);

/**
 * @brief 构造并发送 ARP 应答报文。
 * @param d_ipaddr 目的 IP 地址
 * @param d_ethaddr 目的 MAC 地址
 * @param s_ipaddr 源 IP 地址
 * @param s_ethaddr 源 MAC 地址
 * @param pInfo 接收报文上下文
 */
void arp_reply(const ipaddr_t *  d_ipaddr,
               const ethaddr_t * d_ethaddr,
               const ipaddr_t *  s_ipaddr,
               const ethaddr_t * s_ethaddr,
               NET_RAW_PKG_INFO *pInfo);

/**
 * @brief 更新 ARP 缓存表项。
 * @param ipaddr IP 地址
 * @param ethaddr MAC 地址
 * @param PortType 端口类型
 * @param Src_Slot_ID 源槽位号
 * @param Src_Port_ID 源端口号
 */
void arp_update(const ipaddr_t * ipaddr,
                const ethaddr_t *ethaddr,
                uint32_t         PortType,
                uint32_t         Src_Slot_ID,
                uint32_t         Src_Port_ID);

/**
 * @brief 查询 IP 地址对应的 ARP 表项。
 * @param ethaddr 输出的 MAC 地址
 * @param ipaddr 待查询 IP 地址
 * @param PortType 输出的端口类型
 * @param Src_Slot_ID 输出的源槽位号
 * @param Src_Port_ID 输出的源端口号
 * @return true表示找到表项，false表示未找到
 */
bool arp_check(ethaddr_t *     ethaddr,
               const ipaddr_t *ipaddr,
               uint32_t *      PortType,
               uint32_t *      Src_Slot_ID,
               uint32_t *      Src_Port_ID);

/**
 * @brief 发送 ARP 请求报文。
 * @param d_ipaddr 目标 IP 地址
 * @param net_info 发送 FIFO 句柄
 * @param no 逻辑网口号
 */
void arp_request(const ipaddr_t *d_ipaddr, uint32_t no);

/**
 * @brief 按逻辑网口查询 IP 地址对应的 MAC 地址。
 * @param ethaddr 输出的 MAC 地址
 * @param ipaddr 待查询 IP 地址
 * @param nEthLogNo 以太网逻辑端口号，内网为0，外网依次递增
 * @return true表示找到表项，false表示未找到
 */
bool arp_check_ex(ethaddr_t *ethaddr, const ipaddr_t *ipaddr, uint32_t nEthLogNo);
#ifdef __cplusplus
}
#endif

#endif  //__NET_ARP_H__INCLUDE__