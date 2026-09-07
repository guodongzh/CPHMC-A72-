/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       netlib.c
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议辅助函数实现。
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


/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 根据IP地址生成以太网MAC地址。
 * @param ethaddr 输出的MAC地址
 * @param ipaddr 输入的IP地址
 */


void iptoeth(ethaddr_t *ethaddr, const ipaddr_t *ipaddr)
{
    uint8_t *addr;

    //	aeos_assert(!((NULL == ipaddr) || (NULL == ethaddr)));

    addr = ethaddr->addr;
    if (muti_chkip(*ipaddr))
    {
        addr[0] = 0x01;
        addr[1] = 0x00;
        addr[2] = 0x5e;
        addr[3] = ((*ipaddr) >> 16) & 0x7f;
        addr[4] = ((*ipaddr) >> 8) & 0xff;
        addr[5] = ((*ipaddr)) & 0xff;
    }
    else
    {
        addr[0] = 0x00;
        addr[1] = 0x08;
        addr[2] = (*ipaddr >> 24) & 0xff;
        addr[3] = (*ipaddr >> 16) & 0xff;
        addr[4] = (*ipaddr >> 8) & 0xff;
        addr[5] = (*ipaddr) & 0xff;
    }
    return;
}

/**
 * @brief 将点分十进制字符串转换为IP地址。
 * @param str IP地址字符串
 * @param ipaddr 输出的IP地址
 * @return true表示转换成功，false表示转换失败
 */
bool strtoip(const char *str, ipaddr_t *ipaddr)
{
    uint8_t i, j, m;
    uint8_t len;

    //	aeos_assert(!((NULL == str) || (NULL == ipaddr)));

    *ipaddr = 0;
    len     = strlen(str);
    if (len > 17)
    {
        return false;
    }
    m = 0;
    for (i = 0, j = 0; i < 17; i++)
    {
        if ((str[i] >= '0') && (str[i] <= '9'))
        {
            if (j < 3)
            {
                m = m * 10 + (str[i] - '0');
            }
            else
            {
                return false;
            }
            j++;
        }
        else if ('.' == str[i])
        {
            j       = 0;
            *ipaddr = (*ipaddr << 8) + m;
            m       = 0;
        }
        else if ('\0' == str[i])
        {
            *ipaddr = (*ipaddr << 8) + m;
            break;
        }
        else
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief 填充以太网帧头部的源MAC和目的MAC。
 * @param buf 以太网帧缓冲区
 * @param d_ethaddr 目的MAC地址
 * @param s_ethaddr 源MAC地址
 */
void eth_head(uint32_t *buf, const ethaddr_t *d_ethaddr, const ethaddr_t *s_ethaddr)
{
    const uint8_t *addr;
    uint32_t *     pSend;

    //	aeos_assert(!((NULL == buf) || (NULL == d_ethaddr) || (NULL == s_ethaddr)));

    addr  = d_ethaddr->addr;
    pSend = buf;

    *pSend   &= 0x0000FFFF;
    *pSend++ |= ((uint32_t)(addr[0] & 0xff) << 16) + ((uint32_t)(addr[1] & 0xff) << 24);

    *pSend++ = ((addr[2] & 0xff)) + ((uint32_t)(addr[3] & 0xff) << 8) + ((uint32_t)(addr[4] & 0xff) << 16) + ((uint32_t)(addr[5] & 0xff) << 24);

    *pSend++ = ((s_ethaddr->addr[0] & 0xff)) + ((uint32_t)(s_ethaddr->addr[1] & 0xff) << 8) + ((uint32_t)(s_ethaddr->addr[2] & 0xff) << 16) + ((uint32_t)(s_ethaddr->addr[3] & 0xff) << 24);

    *pSend = ((s_ethaddr->addr[4] & 0xff)) + ((uint32_t)(s_ethaddr->addr[5] & 0xff) << 8);

    return;
}

/**
 * @brief 计算网络报文校验和。
 * @param dp 报文数据起始地址
 * @param len 数据长度，单位为字节
 * @return 16位校验和
 */
uint16_t checksum_net(const uint32_t *dp, uint32_t len)
{
    uint32_t chksum;
    chksum = tcpip_checksum(0, (const uint16_t *)dp, len);
    chksum = (chksum & 0xffff) + ((chksum >> 16) & 0xffff);
    chksum = (chksum & 0xffff) + ((chksum >> 16) & 0xffff);
    chksum = (~chksum) & 0xffff;
    return chksum;
}

/**
 * @brief 累加TCP/IP校验和数据。
 * @param sum 初始累加值
 * @param buffer 数据缓冲区
 * @param bytes 数据长度，单位为字节
 * @return 累加后的校验和值
 */
uint32_t tcpip_checksum(uint32_t sum, const uint16_t *buffer, uint32_t bytes)
{
    uint16_t usTail;
    while (bytes > 1)
    {
        sum   += *buffer++;
        bytes -= sizeof(uint16_t);
    }
    if (bytes)
    {
        usTail                  = 0;
        *((uint8_t *)(&usTail)) = (uint8_t)((*buffer) & 0xFF);
        sum                     += usTail;
    }
    return sum;
}

/**
 * @brief 累加UDP校验和输入数据。
 * @param sum 初始累加值
 * @param buf 数据缓冲区
 * @param len 数据长度，单位为字节
 * @return 累加后的校验和值
 */
static uint32_t checksum_add(uint32_t sum, const uint16_t *buf, uint32_t len)
{
    uint32_t       i;
    uint16_t       value;
    const uint8_t *byte_buf;

    byte_buf = (const uint8_t *)buf; // 字节访问避免对齐问题

    // 累加完整16位块
    for (i = 0; i + 1 < len; i += 2)
    {

        memcpy(&value, byte_buf + i, sizeof(value)); // 非对齐地址安全读取
        sum += htons(value);                         // 保持原字节序处理
    }

    // 奇数字节作为高8位参与计算
    if (i < len)
    {
        sum += (uint16_t)byte_buf[i] << 8;
    }

    return sum;
}

/**
 * @brief 折叠并取反校验和累加值。
 * @param sum 校验和累加值
 * @return 16位校验和
 */
static uint16_t checksum_finalize(uint32_t sum)
{
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

/**
 * @brief 计算UDP校验和。
 * @param eth_send_buff IP报文起始缓冲区
 * @param udp_len UDP报文长度
 * @return UDP校验和，0xFFFF表示参数无效
 */
uint16_t udp_checksum(uint8_t *eth_send_buff, uint16_t udp_len)
{
    pseudo_header_t pseudo;
    uint8_t *       udp_packet;
    uint32_t        sum = 0;

    // 参数有效性检查
    if (!eth_send_buff || udp_len < UDP_HEADER_LEN)
    {
        return 0xFFFF; // 返回无效校验和
    }

    // 构造UDP伪首部
    memcpy(&pseudo.src_ip, eth_send_buff + IP_SRC_OFFSET, 4);
    memcpy(&pseudo.dst_ip, eth_send_buff + IP_DST_OFFSET, 4);
    htonl(pseudo.src_ip);
    htonl(pseudo.dst_ip);
    pseudo.protocol   = htons(0x0011); // UDP协议号
    pseudo.udp_length = *((uint16_t *)(eth_send_buff + UDP_LEN_OFFSET));

    udp_packet = eth_send_buff + UDP_SRCPORT_OFFSET;

    // 分段累加伪首部和UDP数据，避免协议发送路径依赖动态内存
    sum = checksum_add(sum, (uint16_t *)&pseudo, sizeof(pseudo_header_t));
    sum = checksum_add(sum, (uint16_t *)udp_packet, udp_len);

    return checksum_finalize(sum);
}
#ifdef __cplusplus
}
#endif
