/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_arp.c
*@author     xuesen
*@date       2026.05.06
*@brief      ARP报文处理与缓存表管理实现。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"


/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

static uint16_t    ArpSeq;                                               // ARP替换序号
static arp_entry_t ArpTable[MAX_SLOT_NUM][MAX_PORT_NUM][MAX_ARPTAB_NUM]; // ARP缓存表

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 判断IP地址是否为0.0.0.0。
 * @param ip 待检查IP地址
 * @return true表示全零，false表示非全零
 */
static bool is_ipaddr_zero(const ipaddr_t *ip)
{
    const uint8_t *p = (const uint8_t *)ip;
    for (int i = 0; i < sizeof(ipaddr_t); i++)
    {
        if (p[i] != 0)
            return false;
    }
    return true;
}

/**
 * @brief 从ARP报文字节流读取32位IP字段。
 * @param pData 字段起始地址
 * @return 读取到的IP字段值
 */
static ipaddr_t arp_read_u32(const uint8_t *pData)
{
    ipaddr_t value;

    memcpy(&value, pData, sizeof(value));
    return value;
}

/**
 * @brief 判断ARP报文是否属于组网地址或组网MAC。
 * @param pInfo 原始报文信息
 * @return true表示组网报文，false表示普通报文
 */
static bool arp_packet_is_networking(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    ipaddr_t       dst_ip;

    if (!net_profile_enable_networking() || (pInfo == NULL) ||
        (pInfo->pRawPkg == NULL) || (pInfo->nLength < 28))
    {
        return false;
    }

    pRaw   = (const uint8_t *)pInfo->pRawPkg;
    dst_ip = arp_read_u32(pRaw + 24);
    if (ipaddr_cmp(dst_ip, net_ipaddr))
    {
        return true;
    }

    return (memcmp(pInfo->dstMac, net_SrcMacAddr.addr, ENET_MAC_ADDR_LEN) == 0);
}

/**
 * @brief 初始化ARP缓存表。
 */
void arp_init(void)
{
    ArpSeq = 0;
    memset(ArpTable, 0, sizeof(ArpTable));
}

/**
 * @brief 查询IP地址对应的ARP表项。
 * @param ethaddr 输出的MAC地址
 * @param ipaddr 待查询IP地址
 * @param PortType 输出的端口类型
 * @param Src_Slot_ID 输出的源槽位号
 * @param Src_Port_ID 输出的源端口号
 * @return true表示找到表项，false表示未找到
 */
bool arp_check(ethaddr_t *     ethaddr,
               const ipaddr_t *ipaddr,
               uint32_t *      PortType,
               uint32_t *      Src_Slot_ID,
               uint32_t *      Src_Port_ID)
{
    if (!ipaddr || !ethaddr)
        return false;

    for (uint8_t slotid = 0; slotid < MAX_SLOT_NUM; slotid++)
    {
        for (uint8_t portid = 0; portid < MAX_PORT_NUM; portid++)
        {
            arp_entry_t *table = ArpTable[slotid][portid];
            uint32_t     i;

            for (i = 0; i < MAX_ARPTAB_NUM; i++)
            {
                // 跳过未使用项
                if (is_ipaddr_zero(&table[i].ipaddr))
                    continue;

                // 必须匹配IP地址
                if (ipaddr_cmp(*ipaddr, table[i].ipaddr))
                {
                    memcpy(ethaddr, &table[i].ethaddr, sizeof(ethaddr_t));
                    *PortType    = table[i].PortType;
                    *Src_Port_ID = table[i].Src_Port_ID;
                    *Src_Slot_ID = table[i].Src_Slot_ID;
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief 按逻辑网口查询IP地址对应的MAC地址。
 * @param ethaddr 输出的MAC地址
 * @param ipaddr 待查询IP地址
 * @param nEthLogNo 以太网逻辑端口号，内网为0，外网依次递增
 * @return true表示找到表项，false表示未找到
 */
bool arp_check_ex(ethaddr_t *ethaddr, const ipaddr_t *ipaddr, uint32_t nEthLogNo)
{
    uint32_t no;
    uint32_t PortType = 0, Src_Slot_ID = 0, Src_Port_ID = 0;

    if ((ethaddr == NULL) || (ipaddr == NULL))
    {
        return false;
    }

    no = nEthLogNo;
    if (no >= g_nInetMacCount)
    {
        return false;
    }

    // 非本网段目标应通过默认网关查询MAC
    if (!ipaddr_maskcmp(*ipaddr, g_EthIpAddr[no], g_EthMask[no]))
    {
        if (g_EthGateway[no] == 0)
        {
            return false;
        }
        return arp_check(ethaddr, &g_EthGateway[no], &PortType, &Src_Slot_ID, &Src_Port_ID);
    }
    // 本网段目标直接查询目标IP
    else
    {
        return arp_check(ethaddr, ipaddr, &PortType, &Src_Slot_ID, &Src_Port_ID);
    }
}

/**
 * @brief 更新ARP缓存表项。
 * @param ipaddr IP地址
 * @param ethaddr MAC地址
 * @param PortType 端口类型
 * @param Src_Slot_ID 源槽位号
 * @param Src_Port_ID 源端口号
 */
void arp_update(const ipaddr_t * ipaddr,
                const ethaddr_t *ethaddr,
                uint32_t         PortType,
                uint32_t         Src_Slot_ID,
                uint32_t         Src_Port_ID)
{
    if (!ipaddr || !ethaddr)
        return;

    if (Src_Slot_ID >= MAX_SLOT_NUM || Src_Port_ID >= MAX_PORT_NUM)
        return;

    arp_entry_t *table = ArpTable[Src_Slot_ID][Src_Port_ID];
    uint32_t     i;

    // 优先更新已有IP表项
    for (i = 0; i < MAX_ARPTAB_NUM; i++)
    {
        if (!is_ipaddr_zero(&table[i].ipaddr) && ipaddr_cmp(*ipaddr, table[i].ipaddr))
        {
            // MAC相同无需更新
            if (memcmp(table[i].ethaddr.addr, ethaddr->addr, sizeof(ethaddr->addr)) == 0)
                return;

            // MAC变化时同步更新附加端口信息
            table[i].ethaddr     = *ethaddr;
            table[i].PortType    = PortType;
            table[i].Src_Slot_ID = Src_Slot_ID;
            table[i].Src_Port_ID = Src_Port_ID;
            return;
        }
    }

    // 插入空表项
    for (i = 0; i < MAX_ARPTAB_NUM; i++)
    {
        if (is_ipaddr_zero(&table[i].ipaddr))
        {
            table[i].ipaddr      = *ipaddr;
            table[i].ethaddr     = *ethaddr;
            table[i].PortType    = PortType;
            table[i].Src_Slot_ID = Src_Slot_ID;
            table[i].Src_Port_ID = Src_Port_ID;
            return;
        }
    }

    // 表满时覆盖该槽位的固定溢出表项
    arp_entry_t *overflow = &ArpTable[Src_Slot_ID][MAX_PORT_NUM - 1][MAX_ARPTAB_NUM - 1];

    overflow->ipaddr      = *ipaddr;
    overflow->ethaddr     = *ethaddr;
    overflow->PortType    = PortType;
    overflow->Src_Slot_ID = Src_Slot_ID;
    overflow->Src_Port_ID = Src_Port_ID;
}

/**
 * @brief 处理接收到的ARP报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示报文无效
 */
int32_t arp_recv(NET_RAW_PKG_INFO *pInfo)
{
    uint16_t       type;
    ipaddr_t       s_ipaddr,  d_ipaddr;
    ethaddr_t      s_ethaddr, d_ethaddr;
    const uint8_t *pRawData = (const uint8_t *)pInfo->pRawPkg;

    // 检查长度和协议格式
    if (pInfo->nLength < 28)
        return RTN_ERR;

    if (((pRawData[0] << 8) | pRawData[1]) != ARP_HWTYPE_ETH ||
        ((pRawData[2] << 8) | pRawData[3]) != CFG_ETHTYPE_IP ||
        pRawData[4] != 6 || pRawData[5] != 4)
    {
        return RTN_ERR;
    }

    // 解析ARP关键字段
    type = (pRawData[6] << 8) | pRawData[7];
    memcpy(s_ethaddr.addr, &pRawData[8], 6); // 源 MAC
    memcpy(&s_ipaddr, &pRawData[14], 4);     // 源 IP
    memcpy(&d_ipaddr, &pRawData[24], 4);     // 目标 IP

    // 仅响应本核IP或组网IP
    if (ipaddr_cmp(d_ipaddr, self_ipaddr))
    {
        d_ethaddr = self_SrcMacAddr;
    }
    else if (net_profile_enable_networking() && ipaddr_cmp(d_ipaddr, net_ipaddr))
    {
        // 组网地址仅由装置策略指定的核心响应，避免多核重复应答
        if (!net_profile_should_accept_net_ip_icmp())
        {
            return RTN_ERR;
        }
        d_ethaddr = net_SrcMacAddr;
    }
    else
    {
        return RTN_ERR; // 非本机相关IP
    }

    // 更新ARP缓存并响应请求
    arp_update(&s_ipaddr, &s_ethaddr, pInfo->PortType, pInfo->Src_Slot_ID, pInfo->Src_Port_ID);

    if (ARP_REQUEST == type)
    {
        arp_reply(&s_ipaddr, &s_ethaddr, &d_ipaddr, &d_ethaddr, pInfo);
    }

    return RTN_OK;
}


/**
 * @brief 构造并发送ARP响应包。
 * @param d_ipaddr 目的IP地址
 * @param d_ethaddr 目的MAC地址
 * @param s_ipaddr 源IP地址
 * @param s_ethaddr 源MAC地址
 * @param pInfo 接收报文上下文
 */
void arp_reply(const ipaddr_t *  d_ipaddr,
               const ethaddr_t * d_ethaddr,
               const ipaddr_t *  s_ipaddr,
               const ethaddr_t * s_ethaddr,
               NET_RAW_PKG_INFO *pInfo)
{
    uint32_t *                pSend;
    uint8_t                   eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER];
    uint32_t                  report_len;
    net_send_route_t          send_route = {0};
    int32_t                   send_ret;

    // 填充ARP报文内容
    pSend = (uint32_t *)&eth_send_buff[0];

    // ARP头部
    *pSend++ = htonl((ARP_HWTYPE_ETH << 16) | CFG_ETHTYPE_IP);
    *pSend++ = htonl((6 << 24) | (4 << 16) | ARP_REPLY);

    // 发送方MAC与IP前部
    *pSend++ = htonl(
        (s_ethaddr->addr[0] << 24) | (s_ethaddr->addr[1] << 16) | (s_ethaddr->addr[2] << 8) | s_ethaddr->addr[3]);
    *pSend++ = htonl(
        (s_ethaddr->addr[4] << 24) | (s_ethaddr->addr[5] << 16) | (((*s_ipaddr) & 0xFF) << 8) | ((*s_ipaddr >> 8) &
                                                                                                 0xFF));

    // 接收方MAC与IP剩余部分
    *pSend++ = htonl(
        (((*s_ipaddr >> 16) & 0xFF) << 24) | (((*s_ipaddr >> 24) & 0xFF) << 16) | (d_ethaddr->addr[0] << 8) | d_ethaddr
        ->addr[1]);
    *pSend++ = htonl(
        (d_ethaddr->addr[2] << 24) | (d_ethaddr->addr[3] << 16) | (d_ethaddr->addr[4] << 8) | d_ethaddr->addr[5]);

    // 目标IP
    *pSend++ = *d_ipaddr;

    // 由接收端口生成原路回包路由，屏蔽不同平台的驱动结构差异
    report_len                                      = (uint8_t *)pSend - (uint8_t *)&eth_send_buff[0];
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按目的IP或目的MAC是否为组网地址选择发送接口
    if (arp_packet_is_networking(pInfo))
    {
        send_ret = net_port_send_networking(eth_send_buff,
                                            report_len,
                                            d_ethaddr->addr,
                                            CFG_ETHTYPE_ARP,
                                            &send_route);
    }
    else
    {
        send_ret = net_port_send_raw(eth_send_buff,
                                     report_len,
                                     d_ethaddr->addr,
                                     CFG_ETHTYPE_ARP,
                                     &send_route);
    }

    if (send_ret <= 0)
    {
        net_protocol_debug.err_location = 0x08;
        net_protocol_debug.err_number++;
    }
}

/**
 * @brief 构造并发送ARP请求报文。
 * @param d_ipaddr 目标IP地址
 * @param pTxFifo 发送FIFO句柄
 * @param no 逻辑网口号
 */
void arp_do_request(const ipaddr_t *d_ipaddr)
{
    uint32_t *                pSend;
    uint8_t                   eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER];
    uint32_t                  report_len;
    net_send_route_t          send_route = {0};
    int32_t                   send_ret;

    pSend = (uint32_t *)&eth_send_buff[0];

    *pSend++ = ((uint32_t)htons(ARP_HWTYPE_ETH)) + (htons(CFG_ETHTYPE_IP) << 16);

    *pSend++ = ((uint32_t)6) + ((uint32_t)4 << 8) + (htons(ARP_REQUEST) << 16);

    *pSend++ = ((uint32_t)(self_SrcMacAddr.addr[0] & 0xff)) + ((uint32_t)(self_SrcMacAddr.addr[1] & 0xff) << 8) +
               ((uint32_t)(self_SrcMacAddr.addr[2] & 0xff) << 16) + ((self_SrcMacAddr.addr[3] & 0xff) << 24);

    *pSend++ = ((uint32_t)(self_SrcMacAddr.addr[4] & 0xff)) + ((uint32_t)(self_SrcMacAddr.addr[5] & 0xff) << 8) +
               ((htonl(self_ipaddr) << 16) & 0xFFFF0000);

    *pSend++ = ((htonl(self_ipaddr) >> 16) & 0xFFFF) + ((uint32_t)(rec_Remote_MacAddr.addr[0] & 0xff) << 16) +
               ((rec_Remote_MacAddr.addr[1] & 0xff) << 24);

    *pSend++ = ((uint32_t)(rec_Remote_MacAddr.addr[2] & 0xff)) + ((uint32_t)(rec_Remote_MacAddr.addr[3] & 0xff) << 8) +
               ((uint32_t)(rec_Remote_MacAddr.addr[4] & 0xff) << 16) + ((rec_Remote_MacAddr.addr[5] & 0xff) << 24);

    *pSend = htonl(*d_ipaddr);

    //    report_len = pSend - (uint32_t *)(&eth_send_buff) + 1;

    report_len = 28;

    // 主动ARP请求固定从内网0号槽位的首个网口发出
    send_route.port_type   = 0x41;
    send_route.eth_mask[0] = 1;

    send_ret = net_port_send_raw(&eth_send_buff[0],
                                 report_len,
                                 &rec_Remote_MacAddr.addr[0],
                                 CFG_ETHTYPE_ARP,
                                 &send_route);
    if (send_ret <= 0)
    {
        net_protocol_debug.err_location = 0x08;
        net_protocol_debug.err_number++;
    }
}

/**
 * @brief 按目标网段发送ARP请求。
 * @param d_ipaddr 目标IP地址
 * @param pTxFifo 发送FIFO句柄
 * @param no 逻辑网口号
 */
void arp_request(const ipaddr_t *d_ipaddr, uint32_t no)
{
    if ((no >= g_nInetMacCount) || (g_EthGateway[no] == 0))
    {
        arp_do_request(d_ipaddr);
        return;
    }

    // 非本网段目标请求网关MAC
    if (!ipaddr_maskcmp(*d_ipaddr, g_EthIpAddr[no], g_EthMask[no]))
    {
        arp_do_request(&g_EthGateway[no]);
    }
    // 本网段目标直接请求目标MAC
    else
    {
        arp_do_request(d_ipaddr);
    }

    return;
}

#ifdef __cplusplus
}
#endif
