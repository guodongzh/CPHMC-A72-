/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ipc_common.h
 *@author     wenjunf
 *@date       2025.02.17
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.17  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _COMMON_IPC_H
#define _COMMON_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
#include "pcie_fpga.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define __PACKED          __attribute__((packed))

#define CORE0_ID          0
#define CORE1_ID          1
#define CORE2_ID          2
#define CORE3_ID          3
#define CORE53_ID         4

#define CURRENT_SLOT_ID   0         // 当前插件序号
#define CURRENT_CORE_ID   CORE0_ID  // 当前Core序号

#define ENET_IP_ADDR_LEN  4  // IP地址长度
/** \brief MAC address length in bytes */
#define ENET_MAC_ADDR_LEN 6  // MAC地址长度

#define A53_DIR           3

#define SHM_DIR_NUM       5
#define INET_CORE_NUM     4 // 内网核心数量
#define S_CAR_MAX_NUM     2
#define B_CAR_MAX_NUM     32
#define RX_CAR_MAX_NUM    32

// FPGA->CPU:列车最大车厢数量(一节车厢代表一帧以太网报文)，
// 该值由FPGA决定，经过调试后确定该值
#define PCIE_MAX_CAR_NUM 16

#define CAR_LEN_S          128   // car max length
#define CAR_LEN_B          2048  // car max length

#define MAX_PORT_ID        16

#define MULTICAST_MAC_HEAD (0x10)

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct
{
    uint8_t enet_write_pos;
    uint8_t canx_write_pos[3];
} __PACKED rx_ctrl_info_t;

/* core id */
typedef enum
{
    CORE_R0 = 0,
    CORE_R1,
    CORE_R2,
    CORE_R3,
    CORE_MAX
} core_id_t;

/* 核的地址集合 */
typedef struct
{
    uint8_t mac[MAX_PORT_ID][6]; /* 外网 MAC 地址 */
    uint8_t ip[MAX_PORT_ID][4];  /* 外网 IP 地址 (网络字节序) */

    uint8_t lan_mac[6]; /* 内网 MAC 地址 */
    uint8_t lan_ip[4];  /* 内网 IP 地址 (网络字节序) */

    uint8_t send_enet_int_ID; /* 以太网发送中断序号 */
} __PACKED core_addrs_t;

/* 整体的 addr_info */
typedef struct
{
    uint32_t     magic_flag;      /* 固定标志 0x12345678 */
    uint32_t     checksum;        /* 校验和 */
    core_addrs_t cores[CORE_MAX]; /* 每个核的信息 */
    uint8_t net_mac[MAX_PORT_ID][6]; /* 每个外网口的组网 MAC 地址 */
    uint8_t net_ip[MAX_PORT_ID][4];  /* 每个外网口的组网 IP 地址 */
    uint8_t lan_mac[6]; /* 内网口组网 MAC 地址 */
    uint8_t lan_ip[4];  /* 内网口组网 IP 地址*/
} __PACKED addr_info_t;

typedef struct
{
    rx_ctrl_info_t rx_ctrl_info;
    addr_info_t    addr_info;
} __PACKED ipc_shm_info_t;

typedef struct
{
    uint8_t  active_buf_idx;
    uint32_t last_send_cnt;
    uint8_t  frame_num;
} tx_ctrl_t;

typedef struct
{
    uint8_t  active_buf_idx;
    uint32_t last_recv_cnt;
    uint8_t  frame_num;
} rx_ctrl_t;

typedef struct
{
    uint8_t read_pos;  // 当前读取位置
} RxProcessor;

// 发送模式标志
typedef enum
{
    SEND_IPC      = 1 << 0,
    SEND_PHYSICAL = 1 << 1
} send_flag_t;

// 发送控制结构
/* 上下文结构 */
typedef struct
{
    uint8_t     dir_mask;
    send_flag_t flags;
    uint8_t     is_error;
} send_ctx_t;

// 接收模式标志
typedef enum
{
    RECV_LOCAL       = 1 << 0,
    FORWARD_PHYSICAL = 1 << 1
} recv_flag_t;

// 接收控制结构
/* 上下文结构 */
typedef struct
{
    recv_flag_t flags;
    uint8_t     is_error;
} recv_ctx_t;

// 当前Core的MAC、IP、INT_ID
typedef struct
{
    uint8_t srcMac[ENET_MAC_ADDR_LEN];  // 本Core的Mac源地址
    uint8_t src_IP[ENET_IP_ADDR_LEN];   // 本Core的IP源地址
    uint8_t send_enet_int_ID;           // 以太网中断发送序号
} CURRENT_CORE_ENET_MAC_IP_INF_STRUCT;

typedef struct
{
    uint32_t Net_type;  // 网口类型，0x41--内网，0x02-外网
    uint32_t Slot_ID;   // 槽位序号，0~3
    uint32_t Core_ID;   // Core序号，0~3
    uint32_t Net_ID;    // 网口号
    uint16_t send_mode; // 网口的发送模式：0：默认由核0管理转发，1：由本核自己发送
} CURRENT_ENET_INF_STRUCT;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern ipc_shm_info_t ipc_shm_info;

bool check_frame_u32_cnt_add(uint32_t report_cnt_new, uint32_t report_cnt_old);
bool check_frame_u16_cnt_add(uint16_t report_cnt_new, uint16_t report_cnt_old);

uint8_t get_Current_Slot_ID(void);
uint8_t get_Current_Core_ID(void);
uint8_t get_net_config_info(CURRENT_CORE_ENET_MAC_IP_INF_STRUCT *core_enet_inf,
                            CURRENT_ENET_INF_STRUCT              Current_enet_inf);
uint8_t get_locate_enet_inf(CURRENT_CORE_ENET_MAC_IP_INF_STRUCT *p_core_enet_inf,
                            CURRENT_ENET_INF_STRUCT              Current_enet_inf);
void    enet_addr_init(void);
void set_all_port_ip(void);
uint8_t enet_set_mac(uint8_t core_id, uint8_t port_id, const uint8_t mac[6]);
void    set_net_addr_yy(uint8_t yy);
void printf_borad_ip_mac(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _COMMON_IPC_H */