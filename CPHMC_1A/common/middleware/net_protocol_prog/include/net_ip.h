/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_ip.h
*@author     xuesen
*@date       2026.05.06
*@brief      IPv4报文处理与分片重组接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_IP_H__INCLUDE__
#define __NET_IP_H__INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


#define CFG_ETHTYPE_IP 0x0800

#define CFG_NET_TTL    128

// IP头部长度
#if defined(CONFIG_NET_IPV6)
#define NET_IP_HEAD_LEN (40)
#else
#define NET_IP_HEAD_LEN (20)
#endif

#define IP_RF 0x8000U        // 保留分片标志
#define IP_DF 0x4000U        // 禁止分片标志
#define IP_MF 0x2000U        // 后续分片标志
#define IP_OFFMASK 0x1fffU   // 分片偏移掩码

// 编译期常量字节序转换宏
#define PP_HTONS(x) ((uint16_t)((((x) & (uint16_t)0x00ffU) << 8) | (((x) & (uint16_t)0xff00U) >> 8)))
#define PP_NTOHS(x) PP_HTONS(x)
#define PP_HTONL(x) ((((x) & (uint32_t)0x000000ffUL) << 24) | \
                     (((x) & (uint32_t)0x0000ff00UL) <<  8) | \
                     (((x) & (uint32_t)0x00ff0000UL) >>  8) | \
                     (((x) & (uint32_t)0xff000000UL) >> 24))
#define PP_NTOHL(x) PP_HTONL(x)

// IP头字段读取宏
#define IPH_V(hdr)  ((hdr)->_v_hl >> 4)
#define IPH_HL(hdr) ((hdr)->_v_hl & 0x0f)
#define IPH_HL_BYTES(hdr) ((uint8_t )(IPH_HL(hdr) * 4))
#define IPH_TOS(hdr) ((hdr)->_tos)
#define IPH_LEN(hdr) ((hdr)->_len)
#define IPH_ID(hdr) ((hdr)->_id)
#define IPH_OFFSET(hdr) ((hdr)->_offset)
#define IPH_OFFSET_BYTES(hdr) ((uint16_t)((PP_HTONS(IPH_OFFSET(hdr)) & IP_OFFMASK) * 8U))
#define IPH_TTL(hdr) ((hdr)->_ttl)
#define IPH_PROTO(hdr) ((hdr)->_proto)
#define IPH_CHKSUM(hdr) ((hdr)->_chksum)

// IP头字段设置宏
#define IPH_VHL_SET(hdr, v, hl) (hdr)->_v_hl = (uint8_t)((((v) << 4) | (hl)))
#define IPH_TOS_SET(hdr, tos) (hdr)->_tos = (tos)
#define IPH_LEN_SET(hdr, len) (hdr)->_len = (len)
#define IPH_ID_SET(hdr, id) (hdr)->_id = (id)
#define IPH_OFFSET_SET(hdr, off) (hdr)->_offset = (off)
#define IPH_TTL_SET(hdr, ttl) (hdr)->_ttl = (uint8_t)(ttl)
#define IPH_PROTO_SET(hdr, proto) (hdr)->_proto = (uint8_t)(proto)
#define IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)

#define IP_ADDRESSES_AND_ID_MATCH(iphdr1, iphdr2) \
    ((IPH_ID(iphdr1) == IPH_ID(iphdr2)) && \
     ((iphdr1)->src == (iphdr2)->src) && \
     ((iphdr1)->dest == (iphdr2)->dest) && \
     (IPH_PROTO(iphdr1) == IPH_PROTO(iphdr2)))

#define IP_MTU 1500

// IP重组参数
#define IP_REASS_MAX_PBUFS 5     // 单个UDP数据包最大分片数量
#define IP_REASS_MAX_NUM   5     // 最大分片条目
#define IP_REASS_TIMEOUT   30000 // 30秒超时

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

// IPv4头部
typedef struct
{
    // 版本号和头部长度


    uint8_t _v_hl;
    // 服务类型
    uint8_t _tos;
    // 总长度
    uint16_t _len;
    // 标识字段
    uint16_t _id;
    // 分片偏移字段
    uint16_t _offset;
    // 生存时间
    uint8_t _ttl;
    // 上层协议号
    uint8_t _proto;
    // 头部校验和
    uint16_t _chksum;
    // 源和目的 IP 地址
    ipaddr_t src;
    ipaddr_t dest;
} __attribute__ ((__packed__)) ip_hdr;

typedef struct
{
    ip_hdr   first_iphdr;  // 第一个分片的IP头信息
    uint32_t timer;        // 超时计时器
    uint16_t flags;        // 状态标志
    uint16_t received_len; // 已接收数据长度
    uint16_t frag_count;   // 分片计数
    uint8_t  entry_idx;    // 缓冲区索引
} ip_reassdata_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 初始化 IP 分片重组状态。
 */
void ip_reass_init(void);

/**
 * @brief 处理接收到的 IP 报文。
 * @param pInfo 原始报文信息
 * @return RTN_OK表示处理成功，RTN_ERR表示报文无效
 */
int32_t ip_recv(NET_RAW_PKG_INFO * pInfo);

#ifdef __cplusplus
}
#endif

#endif  //__NET_IP_H__INCLUDE__