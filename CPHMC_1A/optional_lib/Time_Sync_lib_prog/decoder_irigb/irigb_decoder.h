#ifndef __IRIG_B_DECODE_H__INCLUDE__
#define __IRIG_B_DECODE_H__INCLUDE__

// IRIGB各码元的宽度

#include "irigb_codeelement.h"
#define IRIGB_0_H              2 * FPGA_CLK_1MS   // 0--高电平个数
#define IRIGB_0_L              8 * FPGA_CLK_1MS   // 0--低电平个数
#define IRIGB_1_H              5 * FPGA_CLK_1MS   // 1--高电平个数
#define IRIGB_1_L              5 * FPGA_CLK_1MS   // 1--低电平个数
#define IRIGB_P_H              8 * FPGA_CLK_1MS   // P码高电平个数
#define IRIGB_P_L              2 * FPGA_CLK_1MS   // P码低电平个数
#define IRIGB_IDLE             10 * FPGA_CLK_1MS  // IDLE电平个数
// ############################################################################
// Share Memory register define
// ############################################################################

#define FPGA_FREQ_MHZ          125  // FPGA时钟  MHz
#define CFG_CORECLK            100  // CPU的内Core的时钟采用PMU 800MHz 除16变为 100MHz

#define CLK_CPU2FPGA(x)        (((x) * FPGA_FREQ_MHZ) / CFG_CORECLK)  // 100MHz -> 125MHz,CPU转FPGA的时钟

// FPGA的时钟
#define FPGA_CLK_1US           ((uint32_t)FPGA_FREQ_MHZ)  // FPGA CLOCK 125MHz
#define FPGA_CLK_1MS           (1000 * FPGA_CLK_1US)
#define FPGA_CLK_1SEC          (1000 * FPGA_CLK_1MS)

// 时标对应单位时间的计数值
#define _1us                   FPGA_CLK_1US
#define _1ms                   (uint32_t)(_1us * 1000)

// B码各个码元的标准宽度
#define _2ms                   (_1ms * 2)    // CLK为FPGA时钟2ms对应的计数值
#define _5ms                   (_1ms * 5)    // CLK为FPGA时钟5ms对应的计数值
#define _8ms                   (_1ms * 8)    // CLK为FPGA时钟8ms对应的计数值
#define MAX_IRIGB_WID_ERR      (_1us * 300)  // 300us，实际IRIG-B脉冲高电平宽度与理想值的允许误差

// CPU的时钟
#define CFG_CLKS_PER_TIMER_CNT (1)                                               // global time clk = 1/2 AHB1 clk.
#define CPU_CLK_1US            ((uint32_t)(CFG_CORECLK / CFG_CLKS_PER_TIMER_CNT))  // 一微秒对应的CPU时钟数
#define CPU_CLK_1MS            (1000 * CPU_CLK_1US)
#define CPU_CLK_1SEC           (1000 * CPU_CLK_1MS)

/// 闰秒信息BIT定义
#define SYNCLK_LS_FRCST_MASK   (0x01)  ///< bit0: 闰秒预告 闰秒之前的所有59秒均置1
#define SYNCLK_LS_DIR_MASK     (0x02)  ///< bit1: 闰秒方向，0=正闰秒 1=负闰秒
#define SYNCLK_LS_POS_MASK     (0x04)  ///< bit2: 正闰秒发生标志 仅在第60"闰秒时刻置1

#define IRIG_B_NO_SIGNAL       (0x00000001)  ///< 无信号, 约50ms无脉冲置位;捕获到脉冲立即清除
#define IRIG_B_ERR_CODEELE     (0x00000002)  ///< 校验错-码元错, 码元序列错置位;解完前75个连续码元(如果有校验还要校验正确)清除
#define IRIG_B_ERR_CHECKSUM    (0x00000004)  ///< 校验错-校验和错, 有校验且校验和错误时置位;下一秒校验和正确时清除
#define IRIG_B_ERR_LEAPSEC     (0x00000008)  ///< 校验错-闰秒标志非法, 检测到非法闰秒标志时置位;下一秒无非法闰秒则清除
#define IRIG_B_ERR_TQ          (0x00000040)  ///< 时间质量无效, 当时间质量为 大于4us 时置位;小于4us时清除

typedef enum
{
    eSCSE_NoEvent = 0,  ///< 无事件
    eSCSE_NewPPSEdge,   ///< 新秒沿
    eSCSE_NewUTCTime,   ///< 新UTC报文，B码有效码元（77个码元）解析完毕

    eSCSE_Count
} eSynClkSrcEvent;  ///< 源时钟事件定义

typedef enum
{
    eIRIGB_Chk_None = 0,  ///< 无校验
    eIRIGB_Chk_Odd,       ///< 奇校验
    eIRIGB_Chk_Even,      ///< 偶校验

    eIRIGB_Chk_Count
} eIrigbCheckMode;  ///< Irigb校验方式

#ifndef WINNT
struct tm
{
    int tm_sec;   /* seconds after the minute - [0,59] */
    int tm_min;   /* minutes after the hour - [0,59] */
    int tm_hour;  /* hours since midnight - [0,23] */
    int tm_mday;  /* day of the month - [1,31] */
    int tm_mon;   /* months since January - [0,11] */
    int tm_year;  /* years since 1900 */
    int tm_wday;  /* days since Sunday - [0,6] */
    int tm_yday;  /* days since January 1 - [0,365] */
    int tm_isdst; /* daylight savings time flag */
};
#endif

typedef struct
{
    uint16_t uSec_100;  // 以100us为单位
    uint8_t  Sec;
    uint8_t  Min;
    uint8_t  Hour;
    uint8_t  Date;
    uint8_t  Month;
    uint16_t Year;
} StruTime;

typedef struct
{
    uint32_t utc_secs;  // Number of seconds since January 1, 1970
    uint32_t fraction;  // Fraction of a second, 1秒的0x1000000等分为单位
    uint32_t uSec_100;  // 100us值
    uint32_t qflags;    // Quality flags, 8 least-significant bits only
} UTC_TIME;

// 本地PPS
typedef struct
{
    uint32_t PPS_nHwClockTag_Rise_new;  // 本地PPS上升沿计数器值(秒宽在正常范围，确认的秒沿)
    uint32_t PPS_nHwClockTag_Rise_old;  // 本地PPS上升沿计数器值
	uint32_t PPS_nHwClockTag_Rise_ack;  // 本地PPS上升沿计数器值(确认值)
    uint32_t nQuartzPropBrif;           // 2个秒沿之间的宽度

} LOCATE_OUT_PPS_INF_STRUCT;

typedef struct
{
    uint32_t nSchedulClk;  // 新的B码码元状态 ，0-重新收B码码元，1-新的B码码元正确、2-该码元解析完毕

    uint32_t                    nHwClockTag_Rise;  // B码上升沿的FPGA计数值
    eIRIGBCodeElement         irigb_ce;
    IRIGB_CODE_ELEMENT_STRUCT irigb_CodeEle;

    uint32_t ceCounter;            // 码元计数器
    uint32_t lastHwClockTag_Rise;  // 上次码元时标
    uint8_t  lastCodeElement;      // 上次收到的码元
    uint32_t lastPPSHwTag;         // 上次PPS的沿时标
    uint32_t nClkPerSec[2];        // 每秒时钟计数,2次确认
    uint32_t nSrcShiftClk;         // 源时钟抖动硬时钟数，单位10ns

    uint8_t  timeQuality;    // 时间质量
    uint8_t  timeShiftSign;  // 时间偏移符号位，时区符号 东、西区
    int8_t   timeShift;      // 时间偏移小时，时区
    uint32_t tmSOC_UTC;      // SOC(UTC时间)
    uint32_t ppsClockTag;    // 准秒沿时刻的硬件时标
    uint32_t nErrorState;    // 解码错误状态
    uint32_t nNoSignalCntr;  // 无信号计时器

    struct tm tmDateTime;  // 日期时间

    uint8_t LeapSecondFlag;  // 闰秒夏时制
    uint8_t validSign;       // 校验位
    uint8_t checksum;        // 校验和

    eIrigbCheckMode checkMode;  // 校验方式

    uint8_t srcCheckOK;   // 对时源校验通过
    uint8_t bcdOneth;     // BCD解码个位暂存
    uint8_t bcdTenth;     // BCD解码十位暂存
    uint8_t bcdBuff[10];  // BCD解码缓冲器

    uint8_t nNormState;  // 获得PPS秒沿标志，0-还没有获得PPS秒沿处于idle状态， 1--获得第2个P码元即PPS标志 ，2--收到完整的B码帧，76个码元

    uint32_t nEdgeRevert_flag;  // 沿反转标志，如300次连续出错复位，程序自动取反
    uint32_t nEdgeErrCntr;      // B码沿出错计数，当77个码元解析正确后清0，沿错误计数器，当连续300个出错，B码沿逻辑自动取反，再重试B码是否正常接收

    CLK_TIME_EDGE issueEdge;  // 收到完整的B码后，确认正确后填写的秒沿信息

    CLK_TIME_EDGE issueTime;  // 在B码接收过程中填写的信息，
} IRIGB_DECODER_LIB_STRUCT;

extern IRIGB_DECODER_LIB_STRUCT  Irigb_Decoder_lib;  // 接收到B码后存放的各码元信息数据库
extern LOCATE_OUT_PPS_INF_STRUCT Locate_out_pps_inf;

void decoder_irigb_init(void);
void irigb_decoder_main(void);

int8_t   irigb_decoder_get_edge(CLK_TIME_EDGE *pValue);
int8_t   irigb_decoder_get_pps(CLK_TIME_EDGE *pValue);
void   UtcTime_To_StruTime(UTC_TIME *pUtcTime_src, StruTime *pStruTime_dest);
void   StruTime_To_UtcTime(StruTime *pStruTime_src, UTC_TIME *pUtcTime_dest);
int8_t RtcGetTimeSoc(uint32_t *p_nRtcSoc);
int8_t RtcSetTimeSoc(const uint32_t *p_nRtcSoc);
#endif