/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       report_interface.h
 *@author     wenjunf
 *@date       2025.07.23
 *@brief      报告管理
 *@par        History
 *Date        Version   Author     Description
 *2025.07.23  1.0       wenjunf    报告管理
 ******************************************************************************/
#ifndef _REPORT_INTERFACE_H__
#define _REPORT_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
#include "ipc_common.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define MAX_REPORT_LENGTH       128  // 存盘报文长度
#define REPORT_SECTOR_NUM       32   // 一个扇区的报告数量
#define MAX_REPORT_INDEX_NUMBER 16   // 索引个数

#define REPORT_TYPE_NUM         6     // 报告类型数量
#define REPORT_OTHER_TYPE_ID    0x35  // 其它报文
#define REPORT_ALM_TYPE_ID      0x36  // 告警报文
#define REPORT_OPERATE_TYPE_ID  0x37  // 操作报文
#define REPORT_RESULT_TYPE_ID   0x38  // 结果报文
#define REPORT_ACTIVE_TYPE_ID   0x39  // 动作报文
#define REPORT_SOE_TYPE_ID      0x40  // SOE报文

#define REPORT_VALID_FLAG       0x80
#define FLASH_ERASED_VAL        0xFF

// 各类型报告最大存储数量
#define MAX_ACTIVE_REPORT_NUM   1024
#define MAX_OPERATE_REPORT_NUM  512
#define MAX_ALM_REPORT_NUM      512
#define MAX_SOE_REPORT_NUM      1024
#define MAX_RESULT_REPORT_NUM   512
#define MAX_OTHER_REPORT_NUM    512
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
// 每个索引结构体
typedef struct
{
    uint32_t Index_ID;  // 在Flash索引序号，从0开始
    uint32_t Soc_Time;  // Soc时间，就是报告中的utcTime时间，以秒为单位
    uint32_t us_time;   // us时间，就是报告中的report_usec时间，以100us为单位
} EACH_INDEX_CONTENT_STRUCT;

typedef struct
{
    uint32_t                  report_type;                             // 报文类型
    uint32_t                  start_Index_ID;                          // 起始的索引序号
    uint32_t                  Index_number;                            // 读索引的个数
    EACH_INDEX_CONTENT_STRUCT index_content[MAX_REPORT_INDEX_NUMBER];  // 索引内容
} REPORT_INDEX_INTERFACE_STRUCT;

// 其他报文，告警报文，操作报文，结果报文，动作报文结构体
#pragma pack(1)
typedef struct _ReportStruct
{
    uint16_t report_index;     // 报告索引，每写一帧报告加1，加到65536溢出回0
    uint8_t  reportValidFlag;  // 80H表示报告有效，FFH表示无效
    uint8_t  msgType;          // 36H-动作报告, 37H-操作报告, 38H-告警报告, 40H-结果报告
    uint8_t  msgResult;
    uint32_t utcTime;      // UTC时间(秒为单位)
    uint8_t  report_year;  // 减去2000后的值
    uint8_t  report_month;
    uint8_t  report_day;
    uint8_t  report_hour;
    uint8_t  report_min;
    uint8_t  report_sec;
    uint16_t report_usec;       // us,以100us为单位
    uint8_t  pluginName[20];    // 插件的ASCII码
    uint8_t  reportSeq;         // 报告序号
    uint8_t  dis_type;			// 报告显示类型
    uint32_t  prevState;         // 变化前值
    uint32_t  currState;         // 变化后值
    uint8_t  msgContent[30];    // 告警/操作报文内容 (30字节)
    uint8_t  reportString[42];  // 报告字符串
    uint8_t  reserved[5];      // 备用
    uint32_t checksum;          // 加和校验范围：1~112，共112/4=28个4字节数加和
} ReportStruct;

#pragma pack()

// Flash 报告存储区域定义
typedef struct _ReportFlashRegion
{
    uint32_t start_addr;           // 起始地址
    uint32_t max_count;            // 最大存储数量
    uint32_t last_erase_sectorID;  // 最后一个被擦除的扇区号
    uint32_t write_idx;            // 报告写索引
    uint32_t read_idx;             // 报告读索引
    uint32_t report_count;         // 有效报告数量
    uint16_t report_new_cnt;       // 报告最新索引计数值
} ReportFlashRegion;

// 报告管理共享内存监视
typedef struct _Report_Manage_Monitor_Inf_Struct
{
    // 报告存储监视
    uint32_t Report_save_index;
    uint32_t Report_save_OK_number;     // 报告存储成功次数
    uint32_t Report_save_err_number;    // 报告存储出错次数
    uint32_t Report_save_err_location;  // 报告存储出错定位

    // 报告索引读取上送监视
    uint32_t Report_Index_read_OK_number;     // 报告索引读取成功次数
    uint32_t Report_Index_read_err_number;    // 报告索引读取出错次数
    uint32_t Report_Index_read_err_location;  // 报告索引读取出错定位

    // 报告内容读取上送监视
    uint32_t Report_content_read_OK_number;     // 报告内容读取成功次数
    uint32_t Report_content_read_err_number;    // 报告内容读取出错次数
    uint32_t Report_content_read_err_location;  // 报告内容读取出错定位

    // 报告管理初始化监视
    uint32_t Report_init_err_number;    // 报告存储出错次数
    uint32_t Report_init_err_location;  // 报告存储出错定位
} Report_Manage_Monitor_Inf_Struct;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern Flash_Handle report_manage_flash_handle;
extern Report_Manage_Monitor_Inf_Struct Report_manage_moniter_inf;

int32_t Save_Report_to_Flash(uint32_t report_type, uint8_t *p_report_content, uint32_t report_len);
int32_t Read_Report_Content(uint32_t report_type, uint32_t report_index_ID, uint8_t *p_report_content);
int32_t report_init(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
