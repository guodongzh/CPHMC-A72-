/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       enet_queue_common.h
*@author     wenjunf
*@date       2024.07.01
*@brief      以太网核间通信底层共用接口
*@par        History
*Date        Version   Author     Description
2024.07.01   1.0       wenjunf    以太网核间通信底层共用接口
******************************************************************************/
#ifndef _ENET_QUEUE_MANAGE_CORE0_
#define _ENET_QUEUE_MANAGE_CORE0_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "enet_queue_manage_core0.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define MAX_METH_SOCKET_ID       10    // 支持最多10个socket_ID
#define MAX_REPORT_TYPES         10    // 每个socket_ID最多支持的报文类型数量
#define RX_QUEUE_SIZE            0x1F  // 接收队列长度
#define TX_QUEUE_SIZE            0x0F  // 发送队列长度

#define CAR_HEADER_LEN           32  // 接收和发送的car header length 32个Byte

#define TXQUEUE_FREAM_HEAD       0x1234  // 以太网列车发送帧头
// 报文类型
#define QUEUE_ENET_MSG_TYPE_FT3  1  // FT3报文
#define QUEUE_ENET_MSG_TYPE_METH 2  // 百兆以太网报文
#define QUEUE_ENET_MSG_TYPE_GETH 3  // 千兆以太网报文
#define QUEUE_ENET_MSG_TYPE_CAN  4  // CAN报文

// Socket_ID 序号定义,其它序号由init_Rx_Socket定义
#define METH_UDP_IP_SOCKET_ID    0  // IP的Socket_ID的序号

#define ENET_QUE_EMPTY           (-(int32_t)(1))
#define ENET_TIMEOUT             (-(int32_t)(2))
#define ENET_INITFAIL            (-(int32_t)(3))
#define ENET_SUM_ERR             (-(int32_t)(4))

/** \brief VLAN tag's Tag Protocol Identifier (TPID) */
#define ETHERTYPE_VLAN_TAG       (0x8100U)

#define MAX_ENET_SLOT_NUMBER     4  // 最大以太网插件个数

/** \brief Max octets in payload */
#define ETH_PAYLOAD_LEN          (1500U)
/** \brief VLAN tag length in bytes */
#define ETH_VLAN_TAG_LEN         (4U)

/* ========================================================================== */
/*                         Structures and Enums                               */
/* ========================================================================== */

typedef struct
{
    uint8_t  dstMac[ENET_MAC_ADDR_LEN];
    uint8_t  srcMac[ENET_MAC_ADDR_LEN];
    uint16_t etherType;
} __attribute__((packed)) EthFrameHeader;

typedef struct
{
    EthFrameHeader hdr;
    uint8_t        payload[ETH_PAYLOAD_LEN + ETH_VLAN_TAG_LEN];
} __attribute__((packed)) EthFrame;

typedef struct
{
    uint8_t  dstMac[ENET_MAC_ADDR_LEN];
    uint8_t  srcMac[ENET_MAC_ADDR_LEN];
    uint16_t tpid;
    uint16_t tci;
    uint16_t etherType;
} __attribute__((packed)) EthVlanFrameHeader;

typedef struct
{
    EthVlanFrameHeader hdr;
    uint8_t            payload[ETH_PAYLOAD_LEN];
} __attribute__((packed)) EthVlanFrame;

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
/*
typedef struct
{
    EthFrameHeader hdr;
    uint8_t        payload[ETH_PAYLOAD_LEN];
    uint16_t       length;  // 有效负载长度（不含帧头）
} EthPacket;
*/

// #################################################################
// 发送队列
typedef struct
{
    enet_car_tx_t packets[TX_QUEUE_SIZE + 1];  // 以太网列车
    uint32_t      rd_point;                    // 读指针
    uint32_t      wr_point;                    // 写指针
} EACH_TX_QUEUE_STRUCT;

typedef struct
{
    uint16_t             cfg_flag;                       // 已配置标志
    uint16_t             send_mode;                      // 网口的发送模式：0：默认由核0管理转发，1：由本核自己发送
    uint16_t             reportTypes[MAX_REPORT_TYPES];  // 记录每个socket_ID的报文类型
    uint16_t             reportTypeCount;                // 报文类型个数
    EACH_TX_QUEUE_STRUCT queue;                          // 对应的循环队列
} EACH_TX_SOCKET_CONFIG_STRUCT;

typedef struct
{
    EACH_TX_SOCKET_CONFIG_STRUCT tx_socket_configs[MAX_METH_SOCKET_ID];  // 已创建的Socket配置
} TX_METH_SOCKET_MANAGE_STRUCT;

// ############################################################################
// 接收队列
typedef struct
{
    enet_car_rx_t packets[RX_QUEUE_SIZE + 1];  // 以太网列车
    uint32_t      rd_point;                    // 读指针
    uint32_t      wr_point;                    // 写指针
} EACH_RX_QUEUE_STRUCT;

typedef struct
{
    uint16_t             cfg_flag;                       // 已配置标志
    uint16_t             reportTypes[MAX_REPORT_TYPES];  // 记录每个socket_ID的报文类型
    uint16_t             reportTypeCount;                // 报文类型个数
    EACH_RX_QUEUE_STRUCT queue;                          // 对应的循环队列
} EACH_RX_SOCKET_CONFIG_STRUCT;

typedef struct
{
    EACH_RX_SOCKET_CONFIG_STRUCT rx_socket_configs[MAX_METH_SOCKET_ID];  // 已创建的Socket配置
} RX_METH_SOCKET_MANAGE_STRUCT;

typedef struct
{
    uint16_t       nEtherType;      // 以太网类型
    uint16_t       nLength;         // 裸包长度|不含MAC头|不含ethertype|不含以太网末尾CRC校验|字节数
    const uint8_t *pRawPkg;         // 纯数据报文，payload指针，不含MAC和以太网类型不含帧头和CRC
    const uint8_t *pMacPkg;         // 纯数据报文，payload指针，含MAC不含帧头和CRC
    uint8_t        rmtMac[6];       // 报文的源MAC地址，也就是发出者的MAC地址
    uint8_t        dstMac[6];       // 报文的目标MAC地址
    uint32_t       nTimeTag;        // 报文到达时刻的FPGA时标
    uint32_t       PortType;        // 端口类型
    uint32_t       Src_Slot_ID;     // 槽位序号
    uint32_t       Src_Port_ID;     // 端口序号
    uint32_t       NetStorm_State;  // 网口风暴状态
} NET_RAW_PKG_INFO;

// 以太网发送端口定义，最多4个插件，每个插件32个端口
typedef struct
{
    uint32_t enet_send_Port_type;             // 以太网发送端口类型
    uint32_t nEthMask[MAX_ENET_SLOT_NUMBER];  // 以太网发送端口，按Bit定义
} ENET_SEND_PORT_DEF_STRUCT;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
// global variables

extern RX_METH_SOCKET_MANAGE_STRUCT Rx_Meth_Socket_Manager;
extern TX_METH_SOCKET_MANAGE_STRUCT Tx_Meth_Socket_Manager;

// functions
typedef bool (*callback_recvfrom_raw)(NET_RAW_PKG_INFO *pInfo);
void     initSocketManager(void);
int32_t  init_Rx_Socket(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type);
int32_t  init_Tx_Socket(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type);
int32_t init_Tx_Socket_with_sendmode(uint32_t socket_ID, uint16_t report_type_number, uint16_t *p_report_type, uint16_t send_mode);
int8_t   inet_fpga_to_rxfifo(void);
int32_t  sendto_raw_socket(uint8_t                   frame_pri,
                           uint32_t                  socket_ID,
                           const uint8_t            *pRawPkg,
                           uint32_t                  nLength,
                           int32_t                   nFlag,
                           ENET_SEND_PORT_DEF_STRUCT enet_send_port_def,
                           const uint8_t            *pDstMac,
                           uint16_t                  nEtherType,
                           uint32_t                  nVLAN);
int32_t sendto_networking_socket(uint8_t                   frame_pri,
                                 uint32_t                  socket_ID,
                                 const uint8_t            *pRawPkg,
                                 uint32_t                  nLength,
                                 int32_t                   nFlag,
                                 ENET_SEND_PORT_DEF_STRUCT enet_send_port_def,
                                 const uint8_t            *pDstMac,
                                 uint16_t                  nEtherType,
                                 uint32_t                  nVLAN);
int32_t  inet_rxfifo_to_app(uint32_t socket_ID_app, callback_recvfrom_raw pCallback, int32_t nCount, void *pEnv);
int8_t   enqueueRxPacket(uint32_t socket_ID, const enet_car_rx_t *p_packet);
int8_t   dequeueRxPacket(uint32_t socket_ID, enet_car_rx_t *packet);
int32_t  recvfrom(uint32_t socket_ID, enet_car_rx_t *packet, uint32_t timeoutMs);
uint32_t cal_enet_send_checksum_prog(enet_car_tx_t *tx_frame);
uint32_t cal_enet_rec_checksum_prog(enet_car_rx_t *rx_frame);
bool     is_inet_frame(uint8_t *d_mac);
bool     is_broadcast_frame(uint8_t *d_mac);
void     pcie_enet_shareram_clear(void);
void     send_to_ethernet(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _ENET_GOOSE_H_ */