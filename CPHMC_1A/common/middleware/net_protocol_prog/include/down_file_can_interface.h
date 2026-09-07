/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       down_file_can_interface.h
*@author     xuesen
*@date       2026.05.06
*@brief      一键下载CAN转发接口定义。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

#ifndef __CAN_INTERFACE_H__INCLUDE__
#define __CAN_INTERFACE_H__INCLUDE__ 1

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/


#if NET_PROFILE_ENABLE_CAN_FORWARD

// 仅为承担升级转发的目标编译CAN队列、命令和任务接口
// CAN网命令
#define DOWN_FILE_CAN_CODE_LOOP_DOWNRADE_CMD 0x92  // 远程升级下行命令
#define DOWN_FILE_CAN_CODE_LOOP_UPGRADE_CMD  0xA2  // 远程升级上行命令

// CAN地址
#define DOWN_FILE_CPU_2_DIO_CAN_GROUP_ID     0x3F  // 主控CPU发给DIO组播
#define DOWN_FILE_PANEL_CAN_ID               0x40  // 面板固定CAN地址
#define DOWN_FILE_MAIN_CTRL_CPU_BASE_CAN_ID  0x41  // 主控CPU基地址 41H~70H
#define DOWN_FILE_MONITOR_TOOLS_CAN_ID       0x7D  // 调试工具

// TFTP一键下载命令
#define OP_TFTP_RRQ                          1
#define OP_TFTP_WRQ                          2
#define OP_TFTP_DATA                         3
#define OP_TFTP_ACK                          4
#define OP_TFTP_ERROR                        5
#define OP_TFTP_OACK                         6
#define OP_TFTP_STATE                        7
#define OP_TFTP_OK                           8
#define OP_TFTP_SUM                          9
#define OP_TFTP_REQ                          10
#define OP_TFTP_ROK                          11

#define DOWN_FILE_MAX_CAN_SEND_QUEUE_NUMBER 0x1F  // 最大CAN发送队列数，考虑到发送32帧 256Byte
#define DOWN_FILE_MAX_CAN_SOCKET_LENGTH     256   // 最大CAN的SOCKET报文长度
#define DOWN_FILE_MAX_APP_BYTE_CAN_SOCKET_NUMBER 0x03
#define DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER  0x3F  // 最大CAN接收队列处理数64

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

// CAN扩展帧ID位域，不包含帧格式标志
typedef struct
{
    uint32_t order_code : 8;    // 命令码 ID28~ID21  此处为最低位
    uint32_t Dest_can_addr : 7; // CAN目的地址 ID20~ID14
    uint32_t Src_can_addr : 7;  // CAN源地址 ID13~ID7
    uint32_t Bak : 1;           // 备用 ID6
    uint32_t continue_flag : 1; // 续帧标志 ID5
    uint32_t socket_ID : 5;     // 帧序号 ID4~ID0
    uint32_t null_X : 3;        // 空        此处为最高位
} DOWN_FILE_CAN_EXTENDER_ID_STRUCT;

// CAN发送队列

typedef struct
{
    uint32_t          can_queue_wr_p;                                          // 队列写指针
    uint32_t          can_queue_rd_p;                                          // 队列读指针
    each_can_report_t can_send_queue[DOWN_FILE_MAX_CAN_SEND_QUEUE_NUMBER + 1]; // CAN缓存区
} DOWN_FILE_ALL_CAN_SEND_QUEUE_STRUCT;

typedef struct
{
    uint32_t can_extender_ID; // CAN扩展ID
    uint32_t send_len;        // 发送长度
    uint8_t  send_buff[DOWN_FILE_MAX_CAN_SOCKET_LENGTH];
} DOWN_FILE_EACH_APP_256BYTE_SEND_BUFF_STRUCT;

typedef struct
{
    uint32_t                                    rd_p; // 读指针
    uint32_t                                    wr_p; // 写指针
    DOWN_FILE_EACH_APP_256BYTE_SEND_BUFF_STRUCT app_send_can_socket[DOWN_FILE_MAX_APP_BYTE_CAN_SOCKET_NUMBER + 1];
} DOWN_FILE_ALL_APP_BYTE_SEND_SOCKET_STRUCT;

// CAN接收队列
typedef struct
{
    uint32_t          can_queue_wr_p;                                             // 队列写指针
    uint32_t          can_queue_loop_rd_p;                                        // 队列循环任务读指针
    each_can_report_t can_rec_queue_buff[DOWN_FILE_MAX_CAN_REC_QUEUE_NUMBER + 1]; // CAN缓存区
} DOWN_FILE_ALL_CAN_RECEIVE_QUEUE_STRUCT;

typedef struct
{
    uint32_t rec_report_len;       // 接收报文长度
    uint32_t expect_rec_socket_ID; // 期望收到的socket序号
    uint32_t can_extender_ID;      // 扩展的ID序号
    uint32_t Src_can_addr;         // 接收Can网报文源Can地址
    uint8_t  can_buff[DOWN_FILE_MAX_CAN_SOCKET_LENGTH];
} DOWN_FILE_ALL_CAN_REC_SOCKET_REPORT_DOWN_FILE_STRUCT;

// 监视信息
typedef struct
{
    uint32_t rec_socket_err_number;
    uint32_t rec_err_locate;

    uint32_t send_socket_err_number;
    uint32_t send_err_locate;
} DOWN_FILE_STATE_MONITOR_CAN_INF_STRUCT;

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief 初始化一键下载CAN1和CAN2转发参数。
 */
void init_down_file_app_can1_param_prog(void);

/**
 * @brief 将一键下载CAN1接收报文压入处理队列。
 * @param p_can_Info_src CAN网原始报文
 * @return true表示已接管处理，false表示非一键下载报文
 */
bool down_file_push_rec_can1_queue_int(each_can_report_t * p_can_Info_src);

/**
 * @brief 将一键下载CAN2接收报文压入面板升级处理队列。
 * @param p_can_Info_src CAN网原始报文
 * @return true表示已接管处理，false表示非一键下载报文
 */
bool down_file_push_rec_can2_queue_int(each_can_report_t * p_can_Info_src);

/**
 * @brief 将CAN短帧拼接为一键下载完整帧。
 * @return true表示收到完整帧，false表示尚未完成
 */
int8_t down_file_app_rec_can1_socket_put_together_loop(void);

/**
 * @brief 将CAN2短帧拼接为面板升级完整帧。
 * @return true表示收到完整帧，false表示尚未完成
 */
int8_t down_file_app_rec_can2_socket_put_together_loop(void);

/**
 * @brief 将以太网升级载荷写入CAN发送socket队列。
 * @param pRawData 以太网TFTP载荷
 * @param dst_can_id 目标CAN ID
 * @param send_len 载荷长度
 * @return 0表示写入成功
 */
uint8_t io_transfer_upgrade_data(uint8_t *pRawData, uint8_t dst_can_id, uint32_t send_len);

/**
 * @brief 将CAN长帧拆分为短帧并提交到底层发送。
 */
void down_file_Can1_send_soft_int_task(void);

/**
 * @brief 将面板升级长帧拆分后通过CAN2提交到底层发送。
 */
void down_file_Can2_send_soft_int_task(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __TCPIP_H__INCLUDE__ */
