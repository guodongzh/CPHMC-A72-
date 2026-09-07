/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_udp.c
*@author     xuesen
*@date       2026.05.06
*@brief      UDP报文收发与分片发送处理实现。
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

callback_recvfrom_udp * m_pCallBackUdp;
callback_recvfrom_udp * m_pCallBackRpc;
callback_tcpip_process *m_pCallBackSntp;

static uint16_t   m_TftpEthPhyNo;

udp_recv_que_t udp_recv_que;

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */


static int32_t udp_send_fragmented(const bystrm *buf,
                                   uint32_t len,
                                   udp_socket_t *udp_Socket,
                                   ethaddr_t *ethaddr,
                                   const net_send_route_t *pRoute);

static uint16_t udp_read_u16_le_unaligned(const uint8_t *p_data)
{
    return (uint16_t)(((uint16_t)p_data[1] << 8) | (uint16_t)p_data[0]);
}

static uint32_t udp_read_u32_le_unaligned(const uint8_t *p_data)
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
 * @brief 注册UDP接收回调函数。
 * @param pCallbackUdp UDP接收回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_callback(callback_recvfrom_udp *pCallbackUdp)
{
    if (pCallbackUdp != NULL)
    {
        m_pCallBackUdp = pCallbackUdp;
        return true;
    }
    return false;
}

/**
 * @brief 注册RPC接收回调函数。
 * @param pCallbackRpc RPC接收回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_rpc(callback_recvfrom_udp *pCallbackRpc)
{
    if (pCallbackRpc != NULL)
    {
        m_pCallBackRpc = pCallbackRpc;
        return true;
    }
    return false;
}

extern uint8_t g_nUseLocalSNTP;
/**
 * @brief 注册SNTP报文处理回调函数。
 * @param pCallbackSntp SNTP报文处理回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_sntp(callback_tcpip_process *pCallbackSntp)
{
    if (pCallbackSntp != NULL)
    {
        m_pCallBackSntp = pCallbackSntp;
        //		g_nUseLocalSNTP = 0x5A;
        return true;
    }
    return false;
}

/**
 * @brief 绑定UDP socket到本地地址。
 * @param psock socket对象
 * @param plocaladdr 本地地址
 * @param ethno 逻辑网口号
 * @return RTN_OK表示成功，RTN_ERR表示失败
 */
int32_t udp_bind(socket_t *psock, const sockaddr_t *plocaladdr, uint16_t ethno)
{
    //	aeos_assert(!((NULL == psock) || (NULL == plocaladdr)));

    if ((psock == NULL) || (plocaladdr == NULL))
        return RTN_ERR;
    if (ethno >= g_nInetMacCount)
        return RTN_ERR;
    if (plocaladdr->ipaddr != g_EthIpAddr[ethno])
        return RTN_ERR;
    psock->ethno      = ethno;
    psock->lcl_port   = plocaladdr->port;
    psock->lcl_ipaddr = plocaladdr->ipaddr;
    return RTN_OK;
}

extern void watchdog_set_progupdating(void);

/**
 * @brief 处理接收到的UDP报文。
 * @param nEthNo 逻辑网口号
 * @param pInfo 原始报文信
 */
void udp_process(uint32_t nEthNo, NET_RAW_PKG_INFO *pInfo)
{
    const uint8_t  *pRawData;
    const uint8_t  *pUdpData;
    socket_t        udpSocket;
    int32_t         nUdpLen;
    uint16_t        udpLen;
    uint16_t        srcPort;
    uint16_t        dstPort;

    if ((pInfo == NULL) || (pInfo->pRawPkg == NULL) || (pInfo->nLength < (NET_IP_HEAD_LEN + CFG_UDPH_LEN)))
    {
        return;
    }
    pRawData = (const uint8_t *)pInfo->pRawPkg;
    pUdpData = pRawData + NET_IP_HEAD_LEN;
    srcPort  = ntohs(udp_read_u16_le_unaligned(pUdpData));
    dstPort  = ntohs(udp_read_u16_le_unaligned(pUdpData + 2));
    udpLen   = ntohs(udp_read_u16_le_unaligned(pUdpData + 4));
    if ((udpLen < CFG_UDPH_LEN) || ((uint32_t)NET_IP_HEAD_LEN + udpLen > pInfo->nLength))
    {
        return;
    }

    switch (dstPort)
    {
    // case TFTP_PORT:
    case TFTP_TID:
        tftp_server(pInfo, nEthNo);
        break;
    case SNTP_PORT:
        if (m_pCallBackSntp != NULL)
        {
            m_pCallBackSntp(nEthNo, pInfo);
            break;
        }
    // 未注册SNTP回调时按普通UDP处理
    default:
        udpSocket.flag = TRUE;
        udpSocket.ethno      = nEthNo;
        udpSocket.rmt_port   = srcPort;                                         // 源PORT
        udpSocket.lcl_port   = dstPort;                                         // 目标PORT
        udpSocket.rmt_ipaddr = ntohl(udp_read_u32_le_unaligned(pRawData + 12)); // 源IP
        udpSocket.lcl_ipaddr = ntohl(udp_read_u32_le_unaligned(pRawData + 16)); // 目标IP
        nUdpLen              = udpLen - CFG_UDPH_LEN;
        udpSocket.PortType    = pInfo->PortType;
        udpSocket.Src_Slot_ID = pInfo->Src_Slot_ID;
        udpSocket.Src_Port_ID = pInfo->Src_Port_ID;
        memcpy(&udpSocket.rmtMac[0], &pInfo->rmtMac[0], 6);
        // if(不分片的包) len= ((pRawData[6]>>16)&0xFFFF)-8;
        if (udp_recv((const bystrm *)(pUdpData + CFG_UDPH_LEN), nUdpLen, &udpSocket) != (uint32_t)nUdpLen)
        {
            // 接收队列满或报文超长时记录丢包，避免高频路径打印日志
            net_protocol_debug.err_location = 0x03;
            net_protocol_debug.err_number++;
        }
        break;
    }
}

/**
 * @brief 将UDP接收数据写入接收队列。
 * @param buf UDP数据起始地址
 * @param len UDP数据长度
 * @param udp_socket UDP连接信息
 * @return 写入的数据长度
 */
uint32_t udp_recv(const bystrm *buf, uint32_t len, socket_t *udp_socket)
{
    uint8_t       next_pointer;
    udp_socket_t *slot;

    if ((buf == NULL) || (udp_socket == NULL))
    {
        return 0;
    }

    slot = &udp_recv_que.udp_data_info[udp_recv_que.w_pointer];
    if (len > sizeof(slot->buf))
    {
        return 0;
    }

    next_pointer = (udp_recv_que.w_pointer + 1) % UDP_RECV_MAX_NUM;
    if (next_pointer == udp_recv_que.r_pointer)
    {
        return 0;
    }

    // 保存UDP数据及其远端属性，供应用侧按需读取
    memcpy(slot->buf, buf, len);
    slot->len        = len;
    slot->rmt_ipaddr = udp_socket->rmt_ipaddr;
    slot->rmt_port   = udp_socket->rmt_port;
    slot->lcl_port   = udp_socket->lcl_port;
    slot->PortType    = udp_socket->PortType;
    slot->Src_Slot_ID = udp_socket->Src_Slot_ID;
    slot->Src_Port_ID = udp_socket->Src_Port_ID;
    memcpy(&slot->rmtMac[0], &udp_socket->rmtMac[0], 6);
    udp_recv_que.w_pointer = next_pointer;
    return len;
}

/**
 * @brief 发送UDP数据。
 * @param buf UDP数据起始地址
 * @param len UDP数据长度
 * @param udp_Socket UDP连接信息
 * @return 发送的数据长度
 */
uint32_t udp_send(const bystrm *buf, uint32_t len, udp_socket_t *udp_Socket)
{
    int32_t send_ret;

    if ((buf == NULL) || (udp_Socket == NULL) || (len > 0xFFFFU - CFG_UDPH_LEN))
    {
        return 0;
    }

    ipaddr_t                  dst_ipaddr = htonl(udp_Socket->rmt_ipaddr);
    ethaddr_t                 ethaddr;
    uint16_t                  nLength;
    uint32_t                  report_len;
    uint8_t                   eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER]; // 发送缓冲区
    uint8_t *                 pSend = eth_send_buff;                   // 数据填充指针
    uint16_t                  udpchecksum;
    uint32_t                  PortType           = 0, Src_Slot_ID = 0, Src_Port_ID = 0;
    net_send_route_t          send_route         = {0};
    bool                      is_sntp            = ((udp_Socket->lcl_port == SNTP_PORT) || (udp_Socket->rmt_port == SNTP_PORT));

    if (is_sntp)
    {
#if defined(SVG_DEV_FUNC)
        // 构网SVG项目：SNTP走内网通讯，交由A53核心转发
        memcpy(ethaddr.addr, inet_macaddr[CORE53_ID], sizeof(ethaddr.addr));
        PortType = INET_PORT_TYPE;
#else
        // 智能终端项目：无内网通讯，直接走外网
        if (!arp_check(&ethaddr, &dst_ipaddr, &PortType, &Src_Slot_ID, &Src_Port_ID))
        {
            arp_request(&dst_ipaddr, m_TftpEthPhyNo);
            iptoeth(&ethaddr, &dst_ipaddr);
        }
#endif
    }
    else
    {
        memcpy(&ethaddr, &udp_Socket->rmtMac[0], 6);
        PortType       = udp_Socket->PortType;
        Src_Slot_ID    = udp_Socket->Src_Slot_ID;
        Src_Port_ID    = udp_Socket->Src_Port_ID;
    }

    // 保存统一发送路由，由平台适配层转换为实际驱动参数
    send_route.port_type = PortType; // 发送以太网类型，内网0x41，外网0x02
    send_route.slot_id   = Src_Slot_ID;
    send_route.port_id   = Src_Port_ID;
#if defined(SVG_DEV_FUNC)
    if (is_sntp == false)
    {
        if (Src_Slot_ID < ARRAYSIZEOF(send_route.eth_mask))
        {
            send_route.eth_mask[Src_Slot_ID] = 1 << Src_Port_ID; // 发送网口号，按Bit定义
        }
    }
#else
    if (Src_Slot_ID < ARRAYSIZEOF(send_route.eth_mask))
    {
        send_route.eth_mask[Src_Slot_ID] = 1 << Src_Port_ID; // 发送网口号，按Bit定义
    }
#endif

    if (len > (IP_MTU - NET_IP_HEAD_LEN - CFG_UDPH_LEN))
    {
        return udp_send_fragmented(buf, len, udp_Socket, &ethaddr, &send_route);
    }

    // 构造IP头部
    nLength = NET_IP_HEAD_LEN + CFG_UDPH_LEN + len;
    ip_head((uint32_t *)pSend, &self_ipaddr, &dst_ipaddr, nLength, CFG_PROTO_UDP); // ip头部组包（20Byte）
    pSend += NET_IP_HEAD_LEN;

    // 构造UDP头部
    *(uint16_t *)pSend       = htons(udp_Socket->lcl_port); // 源端口
    *(uint16_t *)(pSend + 2) = htons(udp_Socket->rmt_port); // 目标端口
    *(uint16_t *)(pSend + 4) = htons(CFG_UDPH_LEN + len);   // UDP 长度
    *(uint16_t *)(pSend + 6) = 0;                           // 校验和占位
    pSend                    += 8;

    // 拷贝UDP数据
    memcpy(pSend, buf, len);
    pSend += len;

    // 计算UDP校验和
    udpchecksum                                        = udp_checksum(eth_send_buff, CFG_UDPH_LEN + len);
    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = htons(udpchecksum);
    //    *(uint16_t *)(eth_send_buff + NET_IP_HEAD_LEN + 6) = 0; //IPV4的UDP校验和为0

    // 计算从IP头开始的报文长度
    report_len = pSend - eth_send_buff;

    // 发送IP报文
    send_ret = net_port_send_raw(
        &eth_send_buff[0],
        report_len,
        &ethaddr.addr[0],
        CFG_ETHTYPE_IP,
        &send_route);

    if (send_ret <= 0)
    {
        return 0;
    }

    return len;
}

/**
 * @brief 分片发送超出MTU的UDP数据。
 * @param buf UDP数据起始地址
 * @param len UDP数据长度
 * @param udp_Socket UDP连接信息
 * @param ethaddr 目的MAC地址
 * @param pRoute 统一发送路由
 * @return 已发送的数据长度
 */
static int32_t udp_send_fragmented(const bystrm *buf,
                                   uint32_t len,
                                   udp_socket_t *udp_Socket,
                                   ethaddr_t *ethaddr,
                                   const net_send_route_t *pRoute)
{
    int32_t  sent_len    = 0;
    uint32_t left        = len;
    uint16_t frag_offset = 0; // 总IP负载偏移
    uint8_t  mf_set      = 1;
    uint32_t ip_len;
    int32_t  last;
    uint8_t  eth_send_buff[MAX_ETH_BUFF_BYTE_NUMBER]; // 发送缓冲区
    ipaddr_t lcl_ipaddr = htonl(self_ipaddr);

    while (left > 0)
    {
        uint16_t udp_frag_size; // 分片大小
        uint16_t ip_payload_len;
        uint16_t tmp;

        // 仅首分片携带UDP头部，后续分片只携带UDP负载
        uint8_t *pSend = eth_send_buff + NET_IP_HEAD_LEN;
        if (frag_offset == 0)
        {
            udp_frag_size            = (left > (IP_MTU - NET_IP_HEAD_LEN - CFG_UDPH_LEN)) ? (IP_MTU - NET_IP_HEAD_LEN - CFG_UDPH_LEN) : left;
            *(uint16_t *)pSend       = htons(udp_Socket->lcl_port);
            *(uint16_t *)(pSend + 2) = htons(udp_Socket->rmt_port);
            *(uint16_t *)(pSend + 4) = htons(CFG_UDPH_LEN + len);
            uint16_t udpchecksum     = 0;
            *(uint16_t *)(pSend + 6) = htons(udpchecksum); // UDP校验和
            ip_payload_len           = udp_frag_size + CFG_UDPH_LEN;
            memcpy(pSend + CFG_UDPH_LEN, buf, udp_frag_size);
            last = (left <= (IP_MTU - NET_IP_HEAD_LEN - CFG_UDPH_LEN));
        }
        else
        {
            udp_frag_size  = (left > (IP_MTU - NET_IP_HEAD_LEN)) ? (IP_MTU - NET_IP_HEAD_LEN) : left;
            ip_payload_len = udp_frag_size;
            memcpy(pSend, buf + frag_offset * 8 - CFG_UDPH_LEN, udp_frag_size);
            last = (left <= (IP_MTU - NET_IP_HEAD_LEN));
        }

        // 构建IP头
        ip_len = NET_IP_HEAD_LEN + ip_payload_len;
        // 设置分片标志和偏移量
        tmp = (IP_OFFMASK & (frag_offset));
        if (!last && mf_set)
        {
            tmp |= IP_MF;
        }
        ip_head2((uint32_t *)eth_send_buff, &lcl_ipaddr, &udp_Socket->rmt_ipaddr, ip_len, tmp, CFG_PROTO_UDP);

        // 发送分片
        int32_t send_ret = net_port_send_raw(
            &eth_send_buff[0],
            ip_len,
            &ethaddr->addr[0],
            CFG_ETHTYPE_IP,
            pRoute);

        if (send_ret <= 0)
        {
            break;
        }

        sent_len    += udp_frag_size;
        left        -= udp_frag_size;
        frag_offset += (IP_MTU - NET_IP_HEAD_LEN) / 8;
    }

    return sent_len;
}
#ifdef __cplusplus
}
#endif
