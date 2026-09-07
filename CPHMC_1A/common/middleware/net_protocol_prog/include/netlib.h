/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       netlib.h
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议辅助函数接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef _CPEMC_2A_R1_DEBUG_KP_NETLIB_H
#define _CPEMC_2A_R1_DEBUG_KP_NETLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


// UDP伪首部


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

typedef struct
{


    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t protocol;
    uint16_t udp_length;
} pseudo_header_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 根据 IP 地址生成以太网 MAC 地址。
 * @param ethaddr 输出的 MAC 地址
 * @param ipaddr 输入的 IP 地址
 */
void iptoeth(ethaddr_t *ethaddr, const ipaddr_t *ipaddr);

/**
 * @brief 将点分十进制字符串转换为 IP 地址。
 * @param str IP 地址字符串
 * @param ipaddr 输出的 IP 地址
 * @return true表示转换成功，false表示转换失败
 */
bool strtoip(const char *str, ipaddr_t *ipaddr);

/**
 * @brief 填充以太网帧头部的源 MAC 和目的 MAC。
 * @param buf 以太网帧缓冲区
 * @param d_ethaddr 目的 MAC 地址
 * @param s_ethaddr 源 MAC 地址
 */
void eth_head(uint32_t *buf, const ethaddr_t *d_ethaddr, const ethaddr_t *s_ethaddr);

/**
 * @brief 计算网络报文校验和。
 * @param dp 报文数据起始地址
 * @param len 数据长度，单位为字节
 * @return 16 位校验和
 */
uint16_t checksum_net(const uint32_t *dp, uint32_t len);

/**
 * @brief 累加 TCP/IP 校验和数据。
 * @param sum 初始累加值
 * @param buffer 数据缓冲区
 * @param bytes 数据长度，单位为字节
 * @return 累加后的校验和值
 */
uint32_t tcpip_checksum(uint32_t sum, const uint16_t *buffer, uint32_t bytes);

/**
 * @brief 计算 UDP 校验和。
 * @param eth_send_buff IP 报文起始缓冲区
 * @param udp_len UDP 报文长度
 * @return UDP 校验和
 */
uint16_t udp_checksum(uint8_t *eth_send_buff, uint16_t udp_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CPEMC_2A_R1_DEBUG_KP_NETLIB_H */