/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       irig_b_interface.h
 *@author     wenjunf
 *@date       2025.09.18
 *@brief      B码对时接口
 *@par        History
 *Date        Version   Author     Description
 *2025.09.18  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _IRIG_B_INTERFACE_H
#define _IRIG_B_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
// 调试Bit信息

// IRIGB码可信状态
#define IRIGB_RELIABLE_STATE_FOLLOW            0x01  // Bit0=1与源时钟保持同步(fellow)
#define IRIGB_RELIABLE_STATE_KEEP              0x02  // Bit1=1 靠晶振守时(keep)
#define IRIGB_RELIABLE_STATE_LOST              0x04  // Bit2=1 失步(lost)
#define IRIGB_RELIABLE_STATE_SRC_JUMP          0x08  // Bit3=1 时钟源跳变,
#define IRIGB_RELIABLE_STATE_SRC_VALID         0x10  // Bit4=1 时钟源有效，当失步时，将时钟源置无效（FPGA的逻辑要求)，判断连续10个有效时钟（抖动小于10us)
#define IRIGB_RELIABLE_STATE_FPGA_PPS_WIDTH_OK 0x20  // Bit5=1 FPGA检测秒脉冲宽度正常

#define T_1970_2000	946656000L	// 本地时间（东8区）到GMT时间1970年1月1日0时0分0秒的差值（秒数）
#define BASE_SECOND2	946684800	// 1970-2000.1.1的秒数（UTC时间）


#define __PACKED                               __attribute__((packed))
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
#if 1
// Irigb对时FPGA给CPU的接口
typedef struct
{
    uint8_t Irigb_edgePolarity;       // B码沿标志   0--下降沿，1--上升沿
    uint8_t Last_Irigb_Rec_Reliably;  // FPGA返回的可信状态
    uint8_t bak3;
    uint8_t bak4;
    uint32_t Irigb_CapValue;            // B码沿32位计数值
    uint32_t PPS_Rising_Edge_CapValue;  // 装置输出PPS上升沿计数器，PPS高电平宽度为10ms
} IRIGB_FPGA_2_CPU_INTERFACE_STRUCT;
#else
// Irigb对时FPGA给CPU的接口
typedef struct
{
    uint8_t Irigb_edgePolarity;       // B码沿标志   0--下降沿，1--上升沿
    uint8_t Last_Irigb_Rec_Reliably;  // FPGA返回的可信状态
    uint8_t bak3;
    uint8_t bak4;
    uint32_t Irigb_CapValue;            // B码沿32位计数值
    uint32_t PPS_Rising_Edge_CapValue;  // 装置输出PPS上升沿计数器，PPS高电平宽度为10ms
    uint32_t Hard_Int_CLkTag;
} IRIGB_FPGA_2_CPU_INTERFACE_STRUCT;
#endif

// B码对时，CPU给FPGA的接口
typedef struct
{
    uint8_t cmd_id;                       // 命令ID
    uint16_t updata_cnt;                  // 更新计数
    uint8_t Last_Irigb_Rec_Reliably;      // 上次B码接收可信信息
    uint8_t Irigb_Reverse_Polarity_flag;  // B码极性取反标志
    uint8_t bak3;
    uint8_t bak4;
} IRIGB_CPU_2_FPGA_INTERFACE_STRUCT;

// FPGA->CPU 上行索引车系统信息字段
typedef struct _pcie_sys_status_data
{
    uint8_t cmd_id;
    uint16_t updata_cnt;
    uint8_t Irigb_edgePolarity;
    uint8_t Last_Irigb_Rec_Reliably;
    uint32_t Irigb_CapValue;
    uint32_t PPS_Rising_Edge_CapValue;
    uint8_t resv2[19];
} __PACKED pcie_sys_status_data_t;

// CPU->FPGA 下行索引车系统本地命令
typedef struct _pcie_sys_cmd_data
{
    uint8_t cmd_id;
    uint16_t updata_cnt;
    uint8_t Last_Irigb_Rec_Reliably;
    uint8_t Irigb_Reverse_Polarity_flag;
    uint8_t resv2[23];
} __PACKED pcie_sys_cmd_data_t;

// 共享内存接口
// 对时接口
typedef struct _CLK_TIME_EDGE
{
    uint32_t nHwClkTag;        // 绝对时间秒沿时刻的硬时钟读数，秒沿时刻的计数值
    uint32_t Hard_Int_CLkTag;  // 硬中断沿的计数值
    uint32_t nHwClkPeriod;     // 两个绝对秒沿之间相隔的硬时钟数[0表示无效]
    uint32_t nUTC;             // 绝对时间秒，对应于格林尼治0时区的绝对UTC时间世纪秒，收到B码的UTC时间后即更新
    uint32_t nFrc;             // 秒等分数，将nSOC含义的秒的等分。[0表示沿时刻]
    uint32_t uSec_100_cnt;     // 100us计数器，用于对时中断周期计算
    uint32_t  nTimeState;      // 时间同步状态表示与直接上级源的同步状态
                               // bit0: 与原时钟保持同步时(follow)置1
                               // bit1: 靠晶振守时状态时(keeping)置1
                               // bit2: 预留
                               // bit3: 源时钟有效时置1
    uint32_t nTimeAlarm;    //对时告警 0正常，1告警

    uint32_t nTimeQuality;  // 对时质量 指与绝对时间的同步偏差 0x0=最好
                            // 0xF=最差，使用B码的时间品质定义，见 eSyncTimeQuality
    uint32_t nLeapSecond;   // 对时闰秒信息 指绝对时间是否为闰秒
                            // bit0: 闰秒预告 闰秒之前的所有59秒均置1
                            // bit1: 闰秒方向，0=正闰秒 1=负闰秒
                            // bit2: 正闰秒发生标志 仅在第60"闰秒时刻置1
    int16_t sTimeZone_min;  // 时区min
} CLK_TIME_EDGE;            // 绝对时间时钟沿

// 对时调试信息
typedef struct _TIME_TEST_INF_STRUCT
{
    uint32_t time_test_bit_state;  // 对时信息按位状态
    uint32_t time_test_inf[39];    // 对时调试信息
} TIME_TEST_INF_STRUCT;

// 对时信息共享内存
typedef struct _SHARE_IRIGB_RAM_STRUCT
{
    CLK_TIME_EDGE Clk_time_Edge;
    TIME_TEST_INF_STRUCT time_test_inf;
} SHARE_IRIGB_RAM_STRUCT;

// utc time
typedef struct
{
    uint32_t utc_secs;  // Number of seconds since January 1, 1970
    uint32_t fraction;  // Fraction of a second, 1秒的0x1000000等分为单位
    uint32_t uSec_100;  // 100us值
    uint32_t qflags;    // Quality flags, 8 least-significant bits only
} APP_UTC_TIME;

typedef struct
{
    uint16_t uSec_100;  // 以100us为单位
    uint8_t  Sec;
    uint8_t  Min;
    uint8_t  Hour;
    uint8_t  Date;
    uint8_t  Month;
    uint16_t Year;
} APP_StruTime;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern SHARE_IRIGB_RAM_STRUCT g_shm_irigb_info;
extern SHARE_IRIGB_RAM_STRUCT g_shm_irigb_info_c6x;


bool Read_Fpga_2_Cpu_Irigb_rec_inf(IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface);
void Write_Fpga_from_Cpu_Irigb_rec_inf(IRIGB_CPU_2_FPGA_INTERFACE_STRUCT *p_Irigb_cpu_2_Fpga_interface);
bool Read_int_Fpga_count(uint32_t *p_int_Fpga_count);
void Write_ShareRam_Clk_time_Edge(CLK_TIME_EDGE *p_Clk_time_Edge);
void Write_ShareRam_Time_test_inf(TIME_TEST_INF_STRUCT *p_time_test_inf);
void Read_ShareRam_Clk_time_Edge(CLK_TIME_EDGE *p_Clk_time_Edge);
void Read_ShareRam_Time_test_inf(TIME_TEST_INF_STRUCT *p_time_test_inf);
void UtcTime_To_StruTime_app(APP_UTC_TIME * pUtcTime_src, APP_StruTime * pStruTime_dest, int16_t nTZShift_min);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _IRIG_B_INTERFACE_H */