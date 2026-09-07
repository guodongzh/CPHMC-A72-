/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_other.h
*@author     xuesen
*@date       2026.05.06
*@brief      网络协议模块公共类型与辅助接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_OTHER__INCLUDE__
#define __NET_OTHER__INCLUDE__


/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#if defined(NET_PORT_AM64X) || defined(NET_PORT_J721E)
#include "ipc_common.h"
#include "enet_queue_manage_core0.h"
#include "enet_queue_manage_othercore.h"
#include "enet_queue_common.h"
#include "inet_queue_common.h"
#endif

#if defined(NET_PORT_C6X0)
#include "file_system.h"
#elif defined(__has_include)
#if __has_include("file_system.h")
#include "file_system.h"
#elif __has_include("file_system/file_system.h")
#include "file_system/file_system.h"
#endif
#endif


/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#define CFG_CORECLK              100  // 核时钟 100Mhz

#define ETH_SEND_FLAG            0  // 以太网发送标志
#define ETH_SEND_nETHMASK        1  // 以太网发送的端口

#define CFG_NET_SOCKS            (7)

#define FPGA_MAC_COUNT           32
#define INET_MAC_COUNT           FPGA_MAC_COUNT
#define ARRAYSIZEOF(a)           (sizeof(a) / sizeof(a[0]))

#define ACP_HEAD_SIZE32          2
#define FIFO_PRE_DATA_ADDR(p)    ((uint32 *)p + ACP_HEAD_SIZE32)  // 收发预留数据偏移

#define DATA_SN(p)               ((*((uint32 *)p) >> 16) & 0x0000FFFF)
#define DATA_LEN(p)              ((*(uint32 *)p) & 0x0000FFFF)                // 8位字节长度
#define DATA_TYPE(p)             ((*(((uint32 *)p + 1)) >> 24) & 0x000000FF)  // 数据类型
#define DATA_VAL_LEN(p)          ((*((uint32 *)p + 1)) & 0x0000FFFF)          // 从SN到校验和的字节数

#define BYTE_TO_WORD(a)          (((a) + 1) >> 1)
#define BYTE_TO_DWORD(a)         (((a) + 3) >> 2)

#define LEN8_TO_SIZE32(a)        (((a) + 3) >> 2)
#define SIZE_32BIT_TO_8BYTE(a)   (((a) + 1) >> 1)
#define SIZE_32BIT_TO_32BYTE(a)  (((a) + 7) >> 3)

#define MAC_DATA_OFFSET_BYTES    (2U)  // mac数据有2Bytes偏移
#define CHECK_SUM_SIZE32         (1U)

#define MAX_ETH_BUFF_BYTE_NUMBER 1500

#define UDP_RECV_MAX_NUM         10

#define RTN_ERR                  -1
#define RTN_OK                   0

// ############################ Tx define
// 以太网发送头长度，不包含ACP头
#define FIFO_PRE_DATA_SIZE32_TX  (4U)
// 32bit对齐
#define RAWLEN8_TO_FIFOSIZE32_TX(len) \
    (ACP_HEAD_SIZE32 + FIFO_PRE_DATA_SIZE32_TX + LEN8_TO_SIZE32(MAC_DATA_OFFSET_BYTES + (len)) + CHECK_SUM_SIZE32)
// 32Bytes对齐
#define RAWLEN8_TO_FIFOSIZE32_32BYPE_TX(len) (8 * SIZE_32BIT_TO_32BYTE(RAWLEN8_TO_FIFOSIZE32_TX(len)))

// 8Bytes对齐
#define RAWLEN8_TO_FIFOSIZE32_8BYPE_TX(len)  (2 * SIZE_32BIT_TO_8BYTE(RAWLEN8_TO_FIFOSIZE32_TX(len)))

// 短整型大小端互换
#define Enet_htons(A) ((((uint16_t)(A) & 0xff00) >> 8) | (((uint16_t)(A) & 0x00ff) << 8))

// 长整型大小端互换
#define Enet_htonl(A)                                                             \
    ((((uint32_t)(A) & 0xff000000) >> 24) | (((uint32_t)(A) & 0x00ff0000) >> 8) | \
     (((uint32_t)(A) & 0x0000ff00) << 8) | (((uint32_t)(A) & 0x000000ff) << 24))

#define htons(s) Enet_htons(s)
#define htonl(l) Enet_htonl(l)

#define ntohs(s) Enet_htons(s)
#define ntohl(l) Enet_htonl(l)

// mac数据有效字节数
#define set_mac_data_val_len_tx(p, v)                                                  \
    do                                                                                 \
    {                                                                                  \
        *((uint32 *)p) &= 0xFFFF0000;                                                  \
        *((uint32 *)p) |= (((((uint32)v) >> 8) & 0xFF) | ((((uint32)v) & 0xFF) << 8)); \
    } while (0)

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

// 基本信息定义
// typedef char               int8_t;
// typedef unsigned char      uint8_t;
// typedef char               sint8_t;
// typedef unsigned short     uint16_t;
// typedef signed short       int16_t;
// typedef unsigned int       uint32_t;
// typedef unsigned long long uint64;
typedef unsigned char bystrm; // 紧凑字节流
// typedef signed int         int32_t;
// typedef float              float32;
// typedef double             float64;
typedef uint32_t ipaddr_t;
typedef int32_t  sockfd_t;

typedef struct
{
    uint8_t addr[6]; // 01 02 03 04 05 06: 网络发送顺序[0]=01,[1]=02...
} ethaddr_t;

typedef enum
{
    LOCAL_UPDATE = 0,
    REMOTE_UPDATE = 1,
    ERROR,
} update_Prop;

// CPU标识，部分单板包含两个Zynq芯片
typedef enum
{
    CPU_0_TAG = 0,
    CPU_1_TAG = 1,
    MAX_CPU_NUM
} eCPU_TAG;

typedef struct
{
    uint32_t nTimeTag;  // 报文到达时刻的FPGA时标
    uint32_t nMacStat;  // 以太网MAC模块给出的状态信息
    uint32_t nReserved; // 预留状态信息
    void *   pEnv;      // 应用自定义上下文
} NET_PKG_ATTACH;

// 先入先出队列(FIFO)的句柄定义
typedef struct acp_buf_t
{
    uint16_t test;
} acp_buf_t;

typedef struct acp_bd_t
{
    uint16_t test;
} acp_bd_t;

typedef struct
{
    ethaddr_t mac_dest_addr; // 目的地址
    ethaddr_t mac_src_addr;  // 源地址地址
    uint16_t  Vlan_tpid;
    uint16_t  Vlan_tci;
    uint16_t  eth_type;                           // 以太网类型
    uint8_t   net_buff[MAX_ETH_BUFF_BYTE_NUMBER]; // 以太网发送缓存区
} VLAN_EACH_ETH_REPORT_BUFF_STRUCT;

typedef struct
{
    ethaddr_t mac_dest_addr;                      // 目的地址
    ethaddr_t mac_src_addr;                       // 源地址地址
    uint16_t  eth_type;                           // 以太网类型
    uint8_t   net_buff[MAX_ETH_BUFF_BYTE_NUMBER]; // 以太网发送缓存区
} EACH_ETH_REPORT_BUFF_STRUCT;

typedef struct sockaddr_t
{
    uint16_t port;
    ipaddr_t ipaddr;
} sockaddr_t;

typedef struct socket_t
{
    bool       flag;        ///< 0(FALSE)表示该socket不可用;非0表示可用
    uint16_t   sendflag;    ///< 发送标志位，默认为0
    uint16_t   ethno;       ///< 以太网口号(逻辑)
    uint16_t   rmt_port;    ///< 远方端口
    uint16_t   lcl_port;    ///< 本地端口
    ipaddr_t   rmt_ipaddr;  ///< 远方ip地址
    ipaddr_t   lcl_ipaddr;  ///< 本地ip地址
    uint8_t    rmtMac[6];   ///< 远方mac地址
    uint32_t   PortType;        // 端口类型
    uint32_t   Src_Slot_ID;     // 槽位序号
    uint32_t   Src_Port_ID;     // 端口序号
} socket_t;

typedef struct
{
    uint16_t   rmt_port;    ///< 远方端口
    uint16_t   lcl_port;    ///< 本地端口
    ipaddr_t   rmt_ipaddr;  ///< 远方ip地址
    uint8_t    buf[1500 * 5]; //和IP分片重组缓存大小一致
    uint32_t   len;
    uint8_t    rmtMac[6];       // 远方mac地址
    uint32_t   PortType;        // 端口类型
    uint32_t   Src_Slot_ID;     // 槽位序号
    uint32_t   Src_Port_ID;     // 端口序号
} udp_socket_t; //UDP数据的接收缓存

typedef struct
{
    udp_socket_t udp_data_info[UDP_RECV_MAX_NUM];
    uint8_t      w_pointer;
    uint8_t      r_pointer;
} udp_recv_que_t;

typedef enum
{
    E_PING_IDLE = 0, // 空闲
    E_PING_CHK_MAC,  // 检查MAC地址
    E_PING_WAIT_ARP, // 等待ARP应答
    E_PING_PING,     // 正在Ping
    E_PING_END,      // Ping完毕，打印统计信息
} ENUM_PING_SM;

#if !defined(NET_PORT_AM64X) && !defined(NET_PORT_J721E)
// 不依赖以太网队列的平台使用协议栈最小接收报文描述
typedef struct NET_RAW_PKG_INFO_Tag
{
    const uint8_t *pRawPkg;
    uint32_t       nLength;
    uint16_t       nEtherType;
    uint32_t       PortType;
    uint32_t       Src_Slot_ID;
    uint32_t       Src_Port_ID;
    uint8_t        dstMac[6];
    uint8_t        rmtMac[6];
} NET_RAW_PKG_INFO;
#endif

// TCP/IP全报文接收回调函数类型
typedef void callback_tcpip_process(int32_t nEthNo, NET_RAW_PKG_INFO *pInfo);

/**
 * @brief UDP数据包接收回调函数类型。
 * @param pSocket 数据包的网口和IP地址信息
 * @param buf UDP纯数据起始地址
 * @param len UDP纯数据长度
 * @return 0表示已处理，-1表示转交系统处理
 */
typedef int32_t callback_recvfrom_udp(const socket_t *, const bystrm *, int32_t);

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/


extern uint32_t  g_nInetMacCount;
extern ethaddr_t g_EthAddr[INET_MAC_COUNT];
extern ethaddr_t g_SrcMacAddr[INET_MAC_COUNT];

extern ipaddr_t g_EthIpAddr[INET_MAC_COUNT];
extern ipaddr_t g_EthMask[INET_MAC_COUNT];
extern ipaddr_t g_EthGateway[INET_MAC_COUNT];

extern ethaddr_t self_SrcMacAddr; // 本网口MAC源地址
extern ipaddr_t  self_ipaddr;     // 本网口IP地址

extern ethaddr_t net_SrcMacAddr; // 组网 MAC 源地址
extern ipaddr_t  net_ipaddr;     // 组网 IP 地址

extern ethaddr_t rec_Remote_MacAddr; // 接收的远端MAC源地址
extern ipaddr_t  rec_Remote_ipaddr;  // 接收的远端IP地址
extern uint8_t   self_canid;         // 本地CAN地址
extern uint8_t   dst_canid;

extern uint32_t sys_clock;

/**
 * @brief 注册UDP接收回调函数。
 * @param pCallbackUdp UDP接收回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_callback(callback_recvfrom_udp *pCallbackUdp);

/**
 * @brief 注册RPC接收回调函数。
 * @param pCallbackRpc RPC接收回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_rpc(callback_recvfrom_udp *pCallbackRpc);

/**
 * @brief 注册SNTP报文处理回调函数。
 * @param pCallbackSntp SNTP报文处理回调函数
 * @return true表示注册成功，false表示注册失败
 */
bool udp_register_sntp(callback_tcpip_process *pCallbackSntp);

/**
 * @brief 获取当前板卡号。
 * @return 板卡号
 */
uint32_t GetBoardNo(void);

/**
 * @brief 获取本地默认 MAC 和 IP 地址。
 * @param p_self_mac 输出的本地 MAC 地址
 * @param p_self_IP 输出的本地 IP 地址
 */
void get_self_Mac_IP(ethaddr_t *p_self_mac, ipaddr_t *p_self_IP);

/**
 * @brief 按文件名查询 FAT 文件属性。
 * @param fileName 文件名
 * @param pResult 输出的文件属性
 * @return true表示找到文件，false表示未找到
 */
bool GetFileAttrByName(const char *fileName, EACH_FLASHFAT_STRUCT *pResult);

/**
 * @brief 计算32位累加校验和。
 * @param data 数据起始地址
 * @param length 数据长度
 * @return 32位累加校验和
 */
uint32_t calculate_checksum(const uint8_t *data, uint32_t length);

#endif
