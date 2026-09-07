/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_tftp.h
*@author     xuesen
*@date       2026.05.06
*@brief      TFTP扩展升级协议接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __NET_TFTP_H__INCLUDE__
#define __NET_TFTP_H__INCLUDE__


/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

#include "tcpip.h"  // ipaddr_t


/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

#define NEW_NET_PROTOCOL  1
#define OLD_NET_PROTOCOL  0

// TFTP基础参数
#define TFTP_PORT         (69)
// #define TFTP_TID          (50022)
#define TFTP_TID          (69)
#define TFTP_FILENAME_LEN (128)
#define TFTP_MODE_LEN     (128)
#define TFTP_FILE_LEN     (128)

#define CRC_SUFFIX_LEN    5  // "_CRC16"占5个字符

// 报文中关键字段的解析偏移
#if NEW_NET_PROTOCOL
#define IP_SRC_OFFSET      (12)                              // IP源地址偏移
#define IP_DST_OFFSET      (16)                              // IP目的地址偏移
#define UDP_SRCPORT_OFFSET (IP_HEADER_LEN)                   // UDP源端口偏移
#define UDP_LEN_OFFSET     (24)                              // UDP长度字段偏移
#define TFTP_DATA_OFFSET   (IP_HEADER_LEN + UDP_HEADER_LEN)  // TFTP数据起始偏移
#define TFTP_CANID_OFFSET  (TFTP_DATA_OFFSET)
#define TFTP_OPCODE_OFFSET (TFTP_CANID_OFFSET + 4)
#define TFTP_BLOCK_OFFSET  (TFTP_OPCODE_OFFSET + 2)
#define TFTP_RDATA_OFFSET  (TFTP_BLOCK_OFFSET + 2)
#define TFTP_SUM_OFFSET    (8)

#define TFTP_HEADER_LEN    (8)
#define TFTP_ACK_LEN       (8)
#define TFTP_ERR_LEN       (8)
#define TFTP_STATE_LEN     (20)
#define TFTP_CKSUM_LEN     (4)
#define TFTP_OK_LEN        (8)
#define TFTP_ROK_LEN       (8)
#define TFTP_LOK_LEN       (8)
#define TFTP_DRES_LEN      (16)
#define TFTP_RST_LEN       (8)

#define TFTP_OP_LEN        (6)
#endif

#define MAX_TFTP_ERR_NUMBER         100

#define TFTP_UPGRADE_STATUS_WRITING 0x1  // 正在升级程序
#define TFTP_UPGRADE_STATUS_IDLE    0x0  // 闲置

#define TFTP_BLOCK_LEN              (1024)             // 默认TFTP块大小
#define TFTP_SEQUENCE_SIZE          ((ulong)(1 << 16)) // 16位块号空间
#define TFTP_MINLEN                 (UDP_CMD2LEN(CFG_TFTPH_LEN))
#define checksum_save(buf, len)     crc_ccitt((buf), (len), 0)


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

enum
{
    TFTP_RRQ = 1,    // 读请求
    TFTP_WRQ = 2,    // 写请求
    TFTP_DATA = 3,   // 数据帧
    TFTP_ACK = 4,    // 确认帧
    TFTP_ERROR = 5,  // 出错帧
    TFTP_REPEAT = 6, // 重发帧
    TFTP_STATE = 7,  // 状态帧
    TFTP_OK = 8,     // 成功帧
    TFTP_SUM = 9,    // 校验和帧（最后一帧）
    TFTP_REQ = 10,   // 请求帧
    TFTP_ROK = 11,   // 成功帧
    TFTP_LCK = 12,   // 链路检测帧
    TFTP_LOK = 13,   // 链路检测成功帧
    TFTP_DIFF = 14,  // 增量下载请求
    TFTP_DRES = 15,  // 增量下载回复
    TFTP_FIF = 16,   // 文件信息帧
    TFTP_STOP = 17,  // 通信终止帧
    TFTP_RST = 18    // 板卡重启帧
};

enum eTftpState
{
    eTftpState_Idle = 0,
    eTftpState_Wrq,
    eTftpState_Rrq,

    eTftpState_Count
};

typedef enum
{
    TFTP_STATE_ERASING = 0xA0,
    TFTP_STATE_ERASE_DONE = 0xA1,

    TFTP_STATE_WRITING = 0xB0,
    TFTP_STATE_WRITE_DONE = 0xB1,

    TFTP_STATE_READING = 0xD0,
    TFTP_STATE_READ_DONE = 0xD1,

    TFTP_STATE_IDLE = 0xC0
} emTFTP_STATE_INFO;

typedef struct
{


    uint32_t tftp_err_location[MAX_TFTP_ERR_NUMBER];
    uint32_t tftp_err_number;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
    uint32_t param5;
} TFTP_SYSTEM_INF_STRUCT;

typedef struct
{
    char     filename[TFTP_FILENAME_LEN];
    char     mode[TFTP_MODE_LEN];
    char     file_len_str[TFTP_FILE_LEN];
    uint32_t file_len;
    uint32_t expected_checksum;
} tftp_wrq_info_t;

typedef struct
{
    char     keyword[TFTP_FILENAME_LEN]; // 这里的 filename 实际上是关键字
    char     mode[TFTP_MODE_LEN];
    uint32_t expected_checksum;
} tftp_rrq_info_t;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/


extern emTFTP_STATE_INFO emTftpState;

/**
 * @brief 处理接收到的TFTP报文。
 * @param pInfo 原始报文信息
 * @param pTxFifo 发送FIFO句柄
 * @param nEthNo 逻辑网口号
 */
void tftp_server(NET_RAW_PKG_INFO *pInfo, uint16_t nEthNo);

/**
 * @brief 发送TFTP升级状态帧。
 * @param state 当前升级状态
 * @param total_size 文件总大小
 * @param current_read_offset 当前读取偏移
 * @param current_write_offset 当前写入偏移
 */
void tftp_send_state(uint16_t state, uint32_t total_size, uint32_t current_read_offset, uint32_t current_write_offset);

/**
 * @brief 执行TFTP 1ms周期维护。
 */
void tftp_1ms_swi_task(void);

/**
 * @brief 获取TFTP停止喂狗标志。
 * @return 1表示停止喂狗，0表示未停止喂狗
 */
uint8_t tftp_get_stop_feed_watchdog_flag(void);

// CAN升级代理专用接口仅向承担代理职责的目标核心开放
#if NET_PROFILE_ENABLE_TFTP_CAN_PROXY
/**
 * @brief 通过TFTP通道发送CAN侧OK帧。
 * @param data CAN侧响应数据
 * @param len 数据长度
 */
void tftp_can_send_ok(const uint8_t *data, uint16_t len);

/**
 * @brief 通过TFTP通道发送CAN侧错误帧。
 * @param data CAN侧响应数据
 * @param len 数据长度
 */
void tftp_can_send_error(const uint8_t *data, uint16_t len);

/**
 * @brief 通过TFTP通道发送CAN侧ACK帧。
 * @param data CAN侧响应数据
 * @param len 数据长度
 */
void tftp_can_send_ack(const uint8_t *data, uint16_t len);

/**
 * @brief 通过TFTP通道发送CAN侧ROK帧。
 * @param data CAN侧响应数据
 * @param len 数据长度
 */
void tftp_can_send_rok(const uint8_t *data, uint16_t len);

/**
 * @brief 将CAN侧响应封装为TFTP报文发送。
 * @param data CAN侧响应数据
 * @param len 数据长度
 */
void tftp_can_send_packet(const uint8_t *data, uint16_t len);
#endif

extern uint8_t g_IsUpdateFile;

extern uint16_t g_update_state;
extern uint32_t g_update_total_size;
extern uint32_t g_update_current_read_offset;
extern uint32_t g_update_current_write_offset;

#endif  //__NET_TFTP_H__INCLUDE__
