/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_icmp.c
*@author     xuesen
*@date       2026.05.06
*@brief      ICMP报文接收与应答处理实现。
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

uint32_t g_IcmpRcvCnt = 0;
uint32_t g_IcmpSndCnt = 0;
uint16_t g_IcmpReqSN  = 0;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

void        icmp_echo_reply(NET_RAW_PKG_INFO *pInfo);
extern void ping_recv(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo);

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 从ICMP/IP报文字节流读取32位字段。
 * @param pData 字段起始地址
 * @return 读取到的32位字段值
 */
static ipaddr_t icmp_read_u32(const uint8_t *pData)
{
    ipaddr_t value;

    memcpy(&value, pData, sizeof(value));
    return value;
}

static uint32_t icmp_read_u32_le_unaligned(const uint8_t *pData)
{
    return ((uint32_t)pData[3] << 24) |
           ((uint32_t)pData[2] << 16) |
           ((uint32_t)pData[1] << 8) |
           (uint32_t)pData[0];
}

/**
 * @brief 判断ICMP报文是否需要走组网发送路径。
 * @param pInfo 原始报文信息
 * @return true表示组网报文，false表示普通报文
 */
static bool icmp_packet_is_networking(NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t *pRaw;
    ipaddr_t       dst_ip;

    if (!net_profile_enable_networking() || (pInfo == NULL) || (pInfo->pRawPkg == NULL))
    {
        return false;
    }

    pRaw   = (const uint8_t *)pInfo->pRawPkg;
    dst_ip = icmp_read_u32(pRaw + IP_DST_OFFSET);
    if (ipaddr_cmp(dst_ip, net_ipaddr))
    {
        return true;
    }

    return (memcmp(pInfo->dstMac, net_SrcMacAddr.addr, ENET_MAC_ADDR_LEN) == 0);
}


/**
 * @brief 处理接收到的ICMP报文。
 * @param pInfo 原始报文信息
 * @param nEthNo 逻辑网口号
 */
void icmp_recv(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo)
{
    const uint8_t *pRawData;

    pRawData = (const uint8_t *)pInfo->pRawPkg;
    g_IcmpRcvCnt++;
    switch (pRawData[NET_IP_HEAD_LEN])
    {
    case ICMP_ECHO_REQUEST:
        icmp_echo_reply(pInfo);
        break;
    case ICMP_ECHO_REPLY:
        ping_recv(pInfo, nEthNo);
        break;
    default:
        break;
    }
}

/**
 * @brief 发送ICMP回显应答报文。
 * @param pInfo 原始报文信息
 */
void icmp_echo_reply(NET_RAW_PKG_INFO *pInfo)
{
    uint8_t         eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER];
    uint32_t *      pSend       = (uint32_t *)eth_send_buff;
    const uint8_t  *rcvdRawData = (const uint8_t *)pInfo->pRawPkg;

    uint32_t                  len,      nSrcIpPkgLen, report_len;
    ipaddr_t                  d_ipaddr, s_ipaddr;
    ethaddr_t                 s_ethaddr;
    net_send_route_t          send_route = {0};
    int32_t                   send_ret;

    // 从IP首部获取接收包长度
    nSrcIpPkgLen = ntohl(icmp_read_u32_le_unaligned(rcvdRawData)) & 0xFFFF;
    len          = ICMP_IP2CMDLEN(nSrcIpPkgLen) - ICMP_ECHO_REQ_LEN;

    // ICMP数据长度边界限制
    if (len <= 18)
        len = 18;
    else if (len >= 256)
        len = 256;

    // 提取IP地址并查询目的MAC
    s_ipaddr = icmp_read_u32_le_unaligned(rcvdRawData + IP_SRC_OFFSET); // 响应包的源 IP
    d_ipaddr = icmp_read_u32_le_unaligned(rcvdRawData + IP_DST_OFFSET); // 响应包的目的 IP

    uint32_t portType = pInfo->PortType;
    uint32_t slotID   = pInfo->Src_Slot_ID;
    uint32_t portID   = pInfo->Src_Port_ID;
    if (!arp_check(&s_ethaddr, &s_ipaddr, &portType, &slotID, &portID))
    {
        // 回显应答优先复用当前接收报文的远端MAC，避免ARP未命中时使用未初始化地址
        memcpy(s_ethaddr.addr, pInfo->rmtMac, sizeof(s_ethaddr.addr));
    }

    // 构建ICMP回显应答主体
    pSend[5] = (ICMP_ECHO_REPLY & 0xFF) << 24;   // Type: 0, Code: 0
    memcpy(&pSend[6], rcvdRawData + NET_IP_HEAD_LEN + 4, len + 4); // 拷贝 Identifier, Sequence, Data

    // 计算ICMP校验和
    pSend[5] &= 0x0000FFFF;
    pSend[5] += htons(checksum_net(&pSend[5], ICMP_LEN(ICMP_ECHO_REPLY_LEN + len)));
    pSend[5] = htonl(pSend[5]);

    // 构建IP头部
    ip_head(&pSend[0],
            &d_ipaddr,
            &s_ipaddr,
            (uint16_t)ICMP_CMD2IPLEN(ICMP_ECHO_REPLY_LEN + len),
            (uint8_t)CFG_PROTO_ICMP);

    // 由接收端口生成原路回包路由，屏蔽不同平台的驱动结构差异
    report_len                                      = len + 28; // ICMP(8) + IP(20) + Data(len)
    net_port_make_route_from_packet(pInfo, &send_route);

    // 按目的IP或目的MAC是否为组网地址选择发送接口
    if (icmp_packet_is_networking(pInfo))
    {
        send_ret = net_port_send_networking(eth_send_buff,
                                            report_len,
                                            s_ethaddr.addr,
                                            CFG_ETHTYPE_IP,
                                            &send_route);
    }
    else
    {
        send_ret = net_port_send_raw(eth_send_buff,
                                     report_len,
                                     s_ethaddr.addr,
                                     CFG_ETHTYPE_IP,
                                     &send_route);
    }

    if (send_ret <= 0)
    {
        net_protocol_debug.err_location = 0x09;
        net_protocol_debug.err_number++;
    }
}


#ifdef __cplusplus
}
#endif
