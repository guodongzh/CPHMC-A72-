/******************************************************************************
 *@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
 *@file       net_ip.c
 *@author     xuesen
 *@date       2026.05.06
 *@brief      IPv4报文处理与分片重组实现。
 *@par        History
 *Date        Version   Author     Description
 *2026.05.06  1.0       xuesen     Initial version
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"

#ifdef __cplusplus
extern "C" {

#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

uint16_t IdIP = 0;

// IP分片重组状态
uint16_t       ip_reass_count = 0;  // IP分片列表中已使用的分片条目数
uint8_t        ip_reass_buffer[IP_REASS_MAX_NUM][IP_MTU * IP_REASS_MAX_PBUFS];
ip_reassdata_t reass_datagrams[IP_REASS_MAX_NUM];
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

static int32_t         ip_process_complete_packet(NET_RAW_PKG_INFO *pInfo);
static bool            ip_process_fragment(NET_RAW_PKG_INFO *pInfo,
                                           uint8_t         **reassembled_data,
                                           uint16_t         *total_len,
                                           ip_hdr          **first_iphdr);
static ip_reassdata_t *ip_reass_find_match(const ip_hdr *fraghdr);
static ip_reassdata_t *ip_reass_get_free_entry(void);

static uint32_t ip_read_u32_le_unaligned(const uint8_t *p_data)
{
    return ((uint32_t)p_data[3] << 24) |
           ((uint32_t)p_data[2] << 16) |
           ((uint32_t)p_data[1] << 8) |
           (uint32_t)p_data[0];
}

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 处理接收到的IP报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示报文无效
 */

int32_t ip_recv(NET_RAW_PKG_INFO *pInfo)
{
    ip_hdr  *iphdr;
    uint16_t ip_len;

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL) || (pInfo->nLength < NET_IP_HEAD_LEN))
    {
        goto DONE;
    }

    iphdr = (ip_hdr *)pInfo->pRawPkg;

    // 仅处理IPv4报文
    if (IPH_V(iphdr) != 4)
    {
        goto DONE;
    }

    if (IPH_HL_BYTES(iphdr) != NET_IP_HEAD_LEN)
    {
        goto DONE;
    }

    ip_len = PP_HTONS(IPH_LEN(iphdr));
    if ((ip_len < NET_IP_HEAD_LEN) || (ip_len > pInfo->nLength))
    {
        goto DONE;
    }

    // 分片报文进入重组流程
    if ((IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)) != 0)
    {
        uint16_t total_len   = 0;
        ip_hdr  *first_iphdr = NULL;
        uint8_t *reassembled_data;

        if (ip_process_fragment(pInfo, &reassembled_data, &total_len, &first_iphdr))
        {
            // 重组完成，构造完整IP包上下文
            NET_RAW_PKG_INFO reassembled_info = *pInfo;

            // 更新重组后的IP头字段
            ip_hdr *new_iphdr = (ip_hdr *)reassembled_data;
            IPH_LEN_SET(new_iphdr, PP_HTONS(total_len));  // 更新总长度
            IPH_OFFSET_SET(new_iphdr, 0);                 // 清除分片标志
            IPH_CHKSUM_SET(new_iphdr, 0);                 // 清零校验和

            reassembled_info.pRawPkg = reassembled_data;
            reassembled_info.nLength = total_len;

            // 直接处理完整IP包，避免递归调用
            return ip_process_complete_packet(&reassembled_info);
        }
        else
        {
            // 分片尚未收齐
            return RTN_OK;
        }
    }
    else
    {
        return ip_process_complete_packet(pInfo);
    }

DONE:
    return RTN_ERR;
}

/**
 * @brief 处理IP分片重组。
 * @param pInfo 原始分片报文信息
 * @param reassembled_data 输出的重组缓冲区
 * @param total_len 输出的重组后IP报文长度
 * @param first_iphdr 输出的首分片IP头
 * @return true表示重组完成，false表示继续等待或处理失败
 */
bool ip_process_fragment(NET_RAW_PKG_INFO *pInfo, uint8_t **reassembled_data, uint16_t *total_len, ip_hdr **first_iphdr)
{
    bool     is_last;
    uint16_t offset, len, ip_hlen, copy_len, copy_offset, expected_len;
    ip_hdr  *fraghdr = (ip_hdr *)pInfo->pRawPkg;

    offset  = IPH_OFFSET_BYTES(fraghdr);  // 分片偏移按8字节单位换算
    len     = PP_HTONS(IPH_LEN(fraghdr));
    ip_hlen = IPH_HL_BYTES(fraghdr);

    if ((ip_hlen > len) || (len > pInfo->nLength))
    {
        net_protocol_debug.err_location = 0x04;
        net_protocol_debug.err_number++;
        return false;
    }

    is_last = (IPH_OFFSET(fraghdr) & PP_NTOHS(IP_MF)) == 0;

    // 查找匹配的重组条目
    ip_reassdata_t *ipr = ip_reass_find_match(fraghdr);

    if (ipr == NULL)
    {
        // 分配新的重组条目
        ipr = ip_reass_get_free_entry();
        if (ipr == NULL)
        {
            net_protocol_debug.err_location = 0x05;
            net_protocol_debug.err_number++;
            return false;
        }

        // 保存首个分片IP头
        memcpy(&ipr->first_iphdr, fraghdr, NET_IP_HEAD_LEN);
        //        ipr->timer = sys_now();
        ipr->timer        = 0;
        ipr->flags        = 1;  // 标记为活跃
        ipr->received_len = 0;
        ipr->frag_count   = 0;
        if (is_last)
        {
            ipr->flags |= 0x02;  // 标记已收到最后分片
        }
    }
    else
    {
        // 第一个分片负责提供完整IP头
        if (offset == 0)
        {
            memcpy(&ipr->first_iphdr, fraghdr, NET_IP_HEAD_LEN);
        }

        // 更新超时计时器
        //        ipr->timer = sys_now();
        ipr->timer = 0;

        if (is_last)
        {
            ipr->flags |= 0x02;  // 标记已收到最后分片
        }
    }

    // 重组缓冲区包含IP头和UDP负载
    uint8_t *buffer = &ip_reass_buffer[ipr->entry_idx][0];

    // 只有首分片携带IP头和UDP头
    uint8_t *payload;
    if (offset == 0)
    {
        payload     = (uint8_t *)pInfo->pRawPkg;
        copy_len    = len;
        copy_offset = 0;
    }
    else
    {
        payload     = (uint8_t *)pInfo->pRawPkg + ip_hlen;
        copy_len    = len - ip_hlen;
        copy_offset = offset + NET_IP_HEAD_LEN;
    }

    if (((uint32_t)copy_offset + copy_len) > (IP_MTU * IP_REASS_MAX_PBUFS))
    {
        net_protocol_debug.err_location = 0x06;
        net_protocol_debug.err_number++;
        return false;
    }

    memcpy(buffer + copy_offset, payload, copy_len);

    ipr->received_len += copy_len;
    ipr->frag_count++;

    expected_len = copy_offset + copy_len;

    if ((ipr->flags & 0x02))
    {
        if (ipr->received_len < expected_len)
        {
            return false;
        }

        // 输出重组结果
        *total_len        = expected_len;
        *reassembled_data = buffer;
        *first_iphdr      = &ipr->first_iphdr;  // 返回首分片IP头

        // 清理重组条目
        memset(ipr, 0, sizeof(ip_reassdata_t));
        if (ip_reass_count > 0)
        {
            ip_reass_count--;
        }

        return true;
    }

    return false;
}

/**
 * @brief 初始化IP分片重组状态。
 */
void ip_reass_init(void)
{
    memset(reass_datagrams, 0, sizeof(reass_datagrams));
    memset(ip_reass_buffer, 0, sizeof(ip_reass_buffer));
    ip_reass_count = 0;
}

/**
 * @brief 查找与当前分片匹配的重组条目。
 * @param fraghdr 当前分片IP头
 * @return 匹配的重组条目，未找到返回NULL
 */
static ip_reassdata_t *ip_reass_find_match(const ip_hdr *fraghdr)
{
    for (int i = 0; i < IP_REASS_MAX_NUM; i++)
    {
        if (reass_datagrams[i].flags != 0 &&
            IP_ADDRESSES_AND_ID_MATCH(&reass_datagrams[i].first_iphdr, fraghdr))
        {
            return &reass_datagrams[i];
        }
    }
    return NULL;
}

/**
 * @brief 获取空闲的IP重组条目。
 * @return 可用的重组条目，未找到返回NULL
 */
static ip_reassdata_t *ip_reass_get_free_entry(void)
{
    //    uint32_t current_time = sys_now();
    uint32_t current_time = 0;

    // 先释放超时条目
    for (int i = 0; i < IP_REASS_MAX_NUM; i++)
    {
        if (reass_datagrams[i].flags != 0 &&
            (current_time - reass_datagrams[i].timer) > IP_REASS_TIMEOUT)
        {
            memset(&reass_datagrams[i], 0, sizeof(ip_reassdata_t));
            if (ip_reass_count > 0)
            {
                ip_reass_count--;
            }
        }
    }

    // 查找完全空闲的条目
    for (int i = 0; i < IP_REASS_MAX_NUM; i++)
    {
        if (reass_datagrams[i].flags == 0)
        {
            // 缓冲区索引与条目索引一致
            reass_datagrams[i].entry_idx = i;
            ip_reass_count++;
            return &reass_datagrams[i];
        }
    }

    return NULL;
}

/**
 * @brief 处理完整IP报文。
 * @param pInfo 原始或重组后的报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示报文无效
 */
static int32_t ip_process_complete_packet(NET_RAW_PKG_INFO *pInfo)
{
    ipaddr_t        s_ipaddr;
    const uint8_t  *pRawData;
    ipaddr_t        d_ipaddr;
    uint32_t        PortType = 0, Src_Slot_ID = 0, Src_Port_ID = 0;
    uint8_t         proto;

    if ((pInfo->pRawPkg == NULL) || (pInfo->nLength < NET_IP_HEAD_LEN))
    {
        goto DONE;
    }

    pRawData = (const uint8_t *)pInfo->pRawPkg;
    s_ipaddr = ip_read_u32_le_unaligned(pRawData + IP_SRC_OFFSET);
    proto    = pRawData[9];

    ip_to(pInfo, &d_ipaddr);
    if (!muti_chkip(d_ipaddr))
    {
        if ((memcmp(pInfo->dstMac, self_SrcMacAddr.addr, ENET_MAC_ADDR_LEN) != 0) &&
            (!net_profile_enable_networking() ||
             (memcmp(pInfo->dstMac, net_SrcMacAddr.addr, ENET_MAC_ADDR_LEN) != 0)))
        {
            goto DONE;
        }
        if (!ipaddr_cmp(d_ipaddr, self_ipaddr) &&
            (!net_profile_enable_networking() || !ipaddr_cmp(d_ipaddr, net_ipaddr)))
        {
            goto DONE;
        }
    }
    else
    {
        if (!muti_chklist(&d_ipaddr))
        {
            goto DONE;
        }
    }

    // 组网ICMP请求仅由装置策略指定的核心处理，避免多核重复响应
    if (net_profile_enable_networking() && ipaddr_cmp(d_ipaddr, net_ipaddr) &&
        !net_profile_should_accept_net_ip_icmp() &&
        (proto == CFG_PROTO_ICMP) && (pRawData[NET_IP_HEAD_LEN] == ICMP_ECHO_REQUEST))
    {
        goto DONE;
    }

    // 收到合法IP包后更新ARP缓存
    PortType    = pInfo->PortType;
    Src_Slot_ID = pInfo->Src_Slot_ID;
    Src_Port_ID = pInfo->Src_Port_ID;

    arp_update(&s_ipaddr, (const ethaddr_t *)&(pInfo->rmtMac), PortType, Src_Slot_ID, Src_Port_ID);

    switch (proto)
    {
    case CFG_PROTO_UDP:
        udp_process(pInfo->Src_Port_ID, pInfo);
        break;
    case CFG_PROTO_ICMP:
        icmp_recv(pInfo, 0);
        break;
    default:
        break;
    }
    return RTN_OK;

DONE:
    return RTN_ERR;
}

/**
 * @brief 构造带分片偏移的IPv4头部。
 * @param pRawData IP头部输出缓冲区
 * @param s_ipaddr 源IP地址
 * @param d_ipaddr 目的IP地址
 * @param len IP报文总长度
 * @param ipoffset 分片标志和偏移
 * @param proto 上层协议号
 */
void ip_head2(uint32_t       *pRawData,
              const ipaddr_t *s_ipaddr,
              const ipaddr_t *d_ipaddr,
              uint16_t        len,
              uint16_t        ipoffset,
              uint8_t         proto)
{
    uint32_t *pSend;

    pSend = pRawData;
    // IPv4头：版本、头长、服务类型和总长度
    *pSend++ = (((uint32_t)htons(len)) << 16) + htons(0x4500);

    // 首分片更新IP标识
    if (ipoffset == 0)
    {
        IdIP++;
    }
    *pSend++ = ((uint32_t)htons((IdIP))) + (((uint32_t)htons(ipoffset)) << 16);

    // TTL、协议号和校验和占位
    *pSend++ = ((uint32_t)(CFG_NET_TTL & 0xff)) + ((uint32_t)(proto & 0xff) << 8);

    // 源和目的IP地址
    *pSend++ = htonl(*s_ipaddr);
    *pSend   = htonl(*d_ipaddr);

    // 计算IP头校验和
    pRawData[2] += ((uint32_t)checksum_net(pRawData, CFG_IPH_LEN)) << 16;
}

/**
 * @brief 构造不分片的IPv4头部。
 * @param pRawData IP头部输出缓冲区
 * @param s_ipaddr 源IP地址
 * @param d_ipaddr 目的IP地址
 * @param len IP报文总长度
 * @param proto 上层协议号
 * @return 写入后的缓冲区指针
 */
uint32_t *ip_head(uint32_t *pRawData, const ipaddr_t *s_ipaddr, const ipaddr_t *d_ipaddr, uint16_t len, uint8_t proto)
{
    uint32_t *pSend;

    pSend = pRawData;
    // IPv4头：版本、头长、服务类型和总长度
    *pSend++ = ((uint32_t)htons(0x4500)) + (htons(len) << 16);

    // IP标识
    *pSend++ = ((uint32_t)htons((IdIP)));
    IdIP++;

    // TTL、协议号和校验和占位
    *pSend++ = ((uint32_t)(CFG_NET_TTL & 0xff)) + ((uint32_t)(proto & 0xff) << 8);

    // 源和目的IP地址
    *pSend++ = *s_ipaddr;
    *pSend   = *d_ipaddr;

    // 计算IP头校验和
    pRawData[2] += ((checksum_net(pRawData, CFG_IPH_LEN)) << 16);

    return pSend;
}

/**
 * @brief 从接收报文中解析目的IP地址。
 * @param pInfo 原始报文信息
 * @param ipaddr 输出的目的IP地址
 */
void ip_to(NET_RAW_PKG_INFO *pInfo, ipaddr_t *ipaddr)
{
    const uint8_t *pRawData;

    // 如果 ipaddr 不为 NULL，提取目标 IP 地址
    if (NULL != ipaddr)
    {
        pRawData = (const uint8_t *)pInfo->pRawPkg;

        // 提取目的IP地址
        ipaddr_copy(*ipaddr, ip_read_u32_le_unaligned(pRawData + IP_DST_OFFSET));
    }

    return;
}

#ifdef __cplusplus
}
#endif
