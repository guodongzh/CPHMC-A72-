/**
 *************************************************************************
 * @file      ipc_scada_rpmsg.h
 * @author    zht
 * @date      2023/11/08
 * @brief     SCADA数据交互 RPMessage 处理, for yt, yk
 * @attention None
 *************************************************************************
 */
#ifndef IPC_SCADA_RPMSG_H_
#define IPC_SCADA_RPMSG_H_

#ifdef __cplusplus
extern "C" {
#endif
#ifdef BUILD_MCU

#include <stdint.h>
#include "ti/osal/TimerP.h"

/*
1. 单点遥调流程：
  1）A53发送消息到R核，将点号和设置值发送给R核，
  2）R核收到后，按点号将接收到的设置值写入到自身的遥调数据区
  3）R核回复A53操作成功或失败

2. 单点遥控流程：
  1）A53发送消息到R核，将点号和遥控指令发送给R核(遥控指令发送正反码，接收端做正反码校验)
  2）R核收到后，按点号将接收到的指令写入到遥控数据区
  3）R核回复A53操作成功或失败

3. 批量遥调流程：
  1）A53发送消息到R核，发送设置点个数(num)和各个点的序号和值
  2）R核收到后，解析num和各个点序号和值，写入到自身的遥调数据区
  3）R核回复A53操作成功或失败

4. 批量遥控流程：
  1）A53发送消息到R核，发送设置点个数(num)和各个点的序号和遥控指令(遥控指令发送正反码，接收端做正反码校验)
  2）R核收到后，解析num和各个点序号和遥控指令，写入到自身的遥控数据区
  3）R核回复A53操作成功或失败
*/

#define IPC_SCADA_MSG_DATA_LEN     (120)

// IPC_SCADA_MSG_t.typ 定义
// A53 -> R0~R3
#define IPC_SCADA_MSG_CMD_YT_1     (1)  // 遥调指令
#define IPC_SCADA_MSG_CMD_YT_N     (2)  // 批量遥调
#define IPC_SCADA_MSG_CMD_YK_1     (3)  // 遥控指令
#define IPC_SCADA_MSG_CMD_YK_N     (4)  // 批量遥控

// R0~R3 -> A53
#define IPC_SCADA_MSG_ACK          (100)  // 确认应答
#define IPC_SCADA_MSG_SOE          (101)  // SOE事件

// 遥控值定义
#define YK_VAL_1_POS               (0xAA)  // 遥控合指令正码
#define YK_VAL_1_INV               (0x55)  // 遥控合指令反码
#define YK_VAL_0_POS               (0xBB)  // 遥控分指令正码
#define YK_VAL_0_INV               (0x44)  // 遥控分指令正码

#define ACK_SUCCED                 (0x26)  // 成功
#define ACK_FAILED                 (0x15)  // 失败

#define ERRCODE_YK_ORD_INVALID     0x01
#define ERRCODE_YK_PTNR_INVALID    0x02
#define ERRCODE_YK_IS_OPERING      0x04
#define ERRCODE_YK_TIMER_BUSY      0x08

/**
 * @brief loacl endpoint
 */
#define LOCAL_ENDPT_FOR_SCADA_RECV 2U

/**
 * @brief remote endpoint
 */
#if defined(BUILD_MCU2_0)
#define REMOTE_ENDPT_FOR_SCADA_SEND 1033U
#elif defined(BUILD_MCU2_1)
#define REMOTE_ENDPT_FOR_SCADA_SEND 1034U
#elif defined(BUILD_MCU3_0)
#define REMOTE_ENDPT_FOR_SCADA_SEND 1035U
#elif defined(BUILD_MCU3_1)
#define REMOTE_ENDPT_FOR_SCADA_SEND 1036U
#endif

// rpmessage
typedef struct
{
    uint8_t dst;  // 目标core
    uint8_t src;  // 源core
    uint8_t res;  // 字节对齐,备用
    uint8_t typ;  // 报文类型

    uint16_t len;  // data 部分数据长度
    uint16_t sum;  // data 部分的校验和
    uint8_t data[IPC_SCADA_MSG_DATA_LEN];

} IPC_SCADA_MSG_t;

// data 部分的具体定义
// A53 -> R0~R3, 单点遥调
typedef struct
{
    uint8_t res[2];  // 字节对齐,备用
    uint16_t Nr;     // 点号
    float val;       // 设置值

} MSG_YT_1_t;

// A53 -> R0~R3, 单点遥控
typedef struct
{
    uint16_t Nr;      // 点号
    uint8_t val_pos;  // 遥控值正码
    uint8_t val_inv;  // 遥控值反码

} MSG_YK_1_t;

// A53 -> R0~R3, 批量遥调, 一次最多设8个点
typedef struct
{
    uint8_t res[3];     // 字节对齐,备用
    uint8_t num;        // 设置点个数
    MSG_YT_1_t val[8];  // 设置值

} MSG_YT_N_t;

// A53 -> R0~R3, 批量遥控, 一次最多设16个点
typedef struct
{
    uint8_t res[3];      // 字节对齐,备用
    uint8_t num;         // 设置点个数
    MSG_YK_1_t val[16];  // 设置值

} MSG_YK_N_t;

// R0~R3 -> A53, 指令应答
typedef struct
{
    uint8_t cmd;      // 应答指令, 对应于接收到的IPC_SCADA_MSG_t.typ
    uint8_t ack;      // 成功或失败
    uint8_t errCode;  // 失败原因
    uint8_t num;      // 操作个数(单点遥控，遥调，是1，批量遥控，遥调，是num)
    uint16_t Nr[16];  // 操作对象号号(操作的点号序列)

} MSG_ACK_t;

typedef struct tagTDATE
{
    uint16_t wYear; /* 2012-2050     */
    uint8_t byMon;  /* 1-12          */
    uint8_t byDay;  /* 1-28,29,30,31 */
    uint8_t byHour; /* 0-23          */
    uint8_t byMin;  /* 0-59          */
    uint16_t wMs;   /* 0-999       */
} TDATE;

// R0~R3 -> A53, SOE
typedef struct
{
    TDATE timestamp;  // 时标

    uint8_t res2[3];  // 字节对齐, 备用
    uint8_t num;      // 变位点个数

    struct
    {
        uint16_t Nr;     // 点序号
        uint8_t diSta1;  // 变位前状态
        uint8_t diSta2;  // 变位后状态
    } DI_CHG[16];

} MSG_SOE_t;

typedef struct
{
    TimerP_Handle timer_handle;
    bool is_active;
    uint16_t pt_nr;  // 遥控点号
    uint16_t ofs;    // offset
    uint8_t bit_no;  // bitNo

} yk_Timer;

void scada_init();
void yt_yk_task(void);
void soe_upload(void);

#ifdef __cplusplus
}
#endif

#endif

#endif